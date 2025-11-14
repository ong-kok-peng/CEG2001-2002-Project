#include <msp430.h> 
#include <stdint.h>
#include "./external_sources/kok_peng/quad_ldrs.h"
#include "./external_sources/kok_peng/uart.h"
#include "./external_sources/zhi_wei/dht22.h"

void msp430f5529_mclk_smclk_8mhz(void);
void set_capturecompare_timer(void);
void set_seconds_stopwatch(void);

volatile uint16_t elapsedSeconds = 0;
volatile uint16_t prevSeconds = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	msp430f5529_mclk_smclk_8mhz(); //MUST RUN FIRST!!
	set_capturecompare_timer();
	set_seconds_stopwatch();
	
	initUART();
	initADCsForLDRs();
	initDHT22();

	__enable_interrupt();

	while (1) {
	    if (elapsedSeconds > 0 && elapsedSeconds-prevSeconds == 1) {
	        //1 seconds interval timer reached, read LDR and rain sensor
	        readLDRsResistance();

            //determine sky ambeint light intensity from averageLDRResistance
            char* ldr_output_str = "";
            if (averageLDRResistance >= 10000) { ldr_output_str = "Night-time darkness"; }
            else if (averageLDRResistance >= 1000 && averageLDRResistance < 10000) { ldr_output_str = "Indoor environment"; }
            else if (averageLDRResistance >= 300 && averageLDRResistance < 1000) { ldr_output_str = "Outdoor no sunlight"; }
            else if (averageLDRResistance >= 100 && averageLDRResistance < 300) { ldr_output_str = "Outdoor moderate sunlight"; }
            else if (averageLDRResistance >= 0 && averageLDRResistance < 100) { ldr_output_str = "Outdoor full sunlight"; }

            uart_printf(ldr_output_str); uart_printf("; ");
            uart_printf("R_LDR_avg:"); integerToUsart(averageLDRResistance); uart_printf(usartValue); uart_printf("\r\n");

	        if (elapsedSeconds % 2 == 0) {
                //2 seconds interval timer reached, read DHT22 sensor
	            beginDHT22Reading();
                readDHT22Reading();

                if (dht22_timedOut) { uart_printf("DHT22 reading timed out!\r\n"); dht22_timedOut = 0; }
                else {
                    uart_printf("humidity:"); integerToUsart(humidity); uart_printf(usartValue);
                    uart_printf(";temperature:"); integerToUsart(temperature); uart_printf(usartValue);
                    uart_printf("\r\n");
                }
            }

	        prevSeconds = elapsedSeconds;
	        uart_printf("\r\n");
	    }

	}

}

//CONFIGURE TIMER_A0 AS CAPTURE/COMPARE TIMER
void set_capturecompare_timer(void) {
    TA0CTL = TASSEL_2 | ID_3 | MC_2 | TACLR;        // SMCLK, /8, continuous 16-bit unsigned counter, clear
}

//CONFIGURE TIMER_A1 AS SECONDS INTERVAL
void set_seconds_stopwatch(void) {
    TA1CTL = TASSEL_1 | ID_3 | MC_1 | TACLR;        // ACLK, /8, count up to TA0CCR0, clear
    TA1CCR0 = 4096;                                 // 1 tick is 0.244ms, so 1s = 4096 ticks
    TA1CCTL0 = CCIE;
}

//TIMER_A1 ISR
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
    elapsedSeconds ++;
}

//CONFIGURE MSP430F5529 TO RUN MCLK AND SMCLK AT 8MHZ!!
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
