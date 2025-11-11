//__delay_cycles() must multiply value by 8!!
//when using timer, makesure prescaler is /8!!

#include <msp430.h>
#include <stdint.h>

volatile char dht22_intervalUp = 0;
volatile uint8_t  dht22_risingDetected = 0;
volatile uint16_t dht22_prevRiseTime = 0;

volatile char dht22_bits[40];
volatile uint8_t dht22_bitcount = 0;
volatile char dht22_dataCollected = 0;

const uint8_t dht22_beginEdgeCount = 2;
volatile uint8_t dht22_edgeCount = 0;
volatile char dht22_timedOut = 0;

void msp430f5529_mclk_smclk_8mhz(void);
void init_dht22_reading(void);
void uart_printf(const char* data);
void integerToUsart(unsigned int integer);
char usartValue[6] = { 0x20, 0x20, 0x20, 0x20, 0x20,'\0' };

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    msp430f5529_mclk_smclk_8mhz();

    //Usart Setting for Serial traces
    P4SEL |= BIT5 + BIT4;            // Set pins for use with UART: P4.5 = RXD, P4.4=TXD
    P4DIR |= BIT4;                    // set as output (RXD)
    P4DIR &= ~BIT5;                   // set as input (TXD) (not used in this code!)

    UCA1CTL1 |= UCSWRST;                        // USCI module in reset mode
    UCA1CTL1 |= UCSSEL_2;                       // SMCLK at 8Mhz!!
    UCA1BR0 = 0x04;                             // Low Byte for 115200 Bd (see User's Guide)
    UCA1BR1 = 0x00;                             // High Byte for 115200 Bd
    UCA1MCTL |= UCBRS_4 | UCBRF_7 | UCOS16;      // Modulation UCBRSx=4, UCBRFx=7, UCOS16 = 1;
    UCA1CTL0 = 0x00;                            // No parity, LSB first, 8-bit, one stop bit
    UCA1CTL1 &= ~UCSWRST;                      // USCI module released for operation

    //timerA configuration
    //ta0ccr2 capture mode timer, capture both rising and falling edge
    TA0CTL = TASSEL_2 | ID_3 | MC_2 | TACLR;        // SMCLK, /8, CONTINUOUS, clear
    TA0CCTL2 = CM_3 | CCIS_0 | SCS | CAP;           // no CCIE yet
    //ta1ccr0 interval every 2s
    TA1CTL = TASSEL_1 | ID_3 | MC_1 | TACLR;        // ACLK, /8, UP, clear
    TA1CCR0 = 8192;                                 // 1 tick is 0.244ms, so 2s = 8192
    TA1CCTL0 = CCIE;

    __enable_interrupt();

    while(1){
        //main program loop, other sensor functions and calculations and logic here

        if (dht22_intervalUp) {
            //2s interval is up, read the dht22

            dht22_intervalUp = 0;
            init_dht22_reading();
            uint16_t dht22_startTime = TA0R;

            while (!dht22_dataCollected) {
                //while data is collecting block to wait until 10ms times out
                //uart_printf("DHT22 reading and reading and reading...\r\n");
                if ((uint16_t)(TA0R - dht22_startTime) >= 10000) { dht22_timedOut = 1; break; }
            }
            if (dht22_timedOut) { uart_printf("DHT22 reading timed out after 10ms!\r\n"); dht22_timedOut = 0; }
            else {

                //take humidity value
                uint16_t humidity_raw = 0;
                int b;
                for (b = 0; b < 16; ++b) {
                    humidity_raw = (humidity_raw << 1) | (dht22_bits[0 + b] & 1); //multiplied by 10
                }

                //take temperature value
                uint16_t temperature_raw = 0;
                for (b = 16; b < 32; ++b) {
                    temperature_raw = (temperature_raw << 1) | (dht22_bits[0 + b] & 1); //multiplied by 10
                }

                uart_printf("humidity:"); integerToUsart(humidity_raw); uart_printf(usartValue);
                uart_printf(";temperature:"); integerToUsart(temperature_raw); uart_printf(usartValue);
                uart_printf("\r\n");
            }
        }
    }

}

//other functions
void msp430f5529_mclk_smclk_8mhz(void) {
    UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
      UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
      UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx

      // Loop until XT1,XT2 & DCO stabilizes - In this case only DCO has to stabilize
      do {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);    // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                             // Clear fault flags
      }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

      __bis_SR_register(SCG0);                  // Disable the FLL control loop
      UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
      UCSCTL2 |= 249;                           // Set DCO Multiplier for 8MHz
                                                // (N + 1) * FLLRef = Fdco
                                                // (249 + 1) * 32768 = 8MHz
      __bic_SR_register(SCG0);                  // Enable the FLL control loop

      // Worst-case settling time for the DCO when the DCO range bits have been
      // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
      // UG for optimization.
      // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
      __delay_cycles(250000);
}

void init_dht22_reading(void) {
    uart_printf("\nNew reading from dht22...\r\n");

    dht22_dataCollected = 0; dht22_bitcount = 0; dht22_edgeCount = 0;
    //initialize dht22 40-bit array
    int a; for (a=0; a < 40; a++) { dht22_bits[a] = 0; }

    P1SEL &= ~BIT3; //enable P1.3 as gpio
    P1DIR |= BIT3; //init P1.3 as output

    P1OUT |= BIT3; //init P1.3 high
    __delay_cycles(800); //delay for some microseconds
    P1OUT &= ~BIT3; //pull P1.3 low
    __delay_cycles(24000); //delay for 3ms
    P1OUT |= BIT3; //pull P1.3 high again
    __delay_cycles(400); //delay for 50us

    P1DIR &= ~BIT3; //set now P1.3 as input
    P1SEL |= BIT3; //disable P1.3 as gpio

    TA0CCTL2 &= ~CCIFG;   // clear stale flag for capture ta0ccr2
    TA0CCTL2 |= CCIE;     // now enable IRQ for capture ta0ccr2
}


/*-----------timerA ISRs--------------*/
//timerA1 2s dht22 interval ISR
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
    dht22_intervalUp = 1;
}
//timerA0 capture dht22 ISR
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void) {
    switch (TA0IV) {
    case TA0IV_TACCR2: {
        uint16_t dht22_timestamp = TA0CCR2;

        // If we missed an edge, resync this bit
        if (TA0CCTL2 & COV) { TA0CCTL2 &= ~COV; dht22_risingDetected = 0; }

        // Count edges and skip the 2 presence edges
        dht22_edgeCount++;
        if (dht22_edgeCount <= dht22_beginEdgeCount) { dht22_risingDetected = 0; break; }

        // Read pin level: high => we just captured a rising edge; low => falling
        uint8_t dht22_riseOrFall = (P1IN & BIT3) ? 1 : 0;

        if (dht22_riseOrFall) {
            // RISING: mark start of HIGH pulse
            dht22_prevRiseTime = dht22_timestamp;
            dht22_risingDetected = 1;
        } else {
            // FALLING: HIGH pulse ended → measure width
            if (dht22_risingDetected && dht22_bitcount < 40) {
                uint16_t dt = (uint16_t)(dht22_timestamp - dht22_prevRiseTime);

                // small sanity window to drop glitches
                if (dt >= 10 && dt <= 200) {
                    dht22_bits[dht22_bitcount++] = (dt >= 50) ? 1 : 0; // threshold ~50 µs
                    if (dht22_bitcount == 40) {
                        dht22_dataCollected = 1;
                        TA0CCTL2 &= ~CCIE;     // done
                    }
                }
            }
            dht22_risingDetected = 0;
        }
    } break;
    default: break;
    }
}


//UART functions
void uart_printf(const char* data)
{
    volatile char i = 0;
    while (data[i] != '\0') {         // Check for end of string '\0'
        UCA1TXBUF = data[i];        // Put character to buffer
        while (UCA1STAT & UCBUSY);   // Wait until char is sent
        i++;
    }

}

void integerToUsart(unsigned int integer)
{
    char tenthousands, thousands, hundreds, tens, ones;

    tenthousands = integer / 10000;
    usartValue[0] = (char)(tenthousands + 0x30);

    thousands = integer % 10000 / 1000;
    usartValue[1] = (thousands + 0x30);

    hundreds = integer % 1000 / 100;
    usartValue[2] = (hundreds + 0x30);

    tens = integer % 100 / 10;
    usartValue[3] = (tens + 0x30);

    ones = integer % 10;
    usartValue[4] = (ones + 0x30);

}
