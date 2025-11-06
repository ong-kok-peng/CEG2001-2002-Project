#include <msp430.h> 
#include <stdint.h>

volatile int adc_vals[4] = {0, 0, 0, 0};
const int volt_dvr_r1s[4] = {490, 490, 490, 490}; // adjust to actual r1 values for each volt divider

void light_ldr_measurement();
void uart_printf(const char* data);
void integerToUsart(unsigned int integer);
char usartValue[6] = { 0x20, 0x20, 0x20, 0x20, 0x20,'\0' };

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	//ADC REGISTERS
	P6SEL |= BIT0 | BIT1 | BIT2 | BIT3;                   // P6.0 thru P6.3 ADC option select

	ADC12CTL0 = ADC12SHT02 + ADC12MSC + ADC12ON;          // Sampling time, ADC12 on
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_1;                 // Use sampling timer, with sequence of ADC channels

    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;               // Vr+=AVcc and Vr-=AVss; Select Channel A0
    ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_1;               // select channel A1
    ADC12MCTL2 = ADC12SREF_0 + ADC12INCH_2;               // select channeL A2
    ADC12MCTL3 = ADC12SREF_0 + ADC12INCH_3 + ADC12EOS;    // select channel A3

    ADC12IE |= ADC12IE3;                         // Enable ADC interrupt for last ADC channel as EOS
    ADC12CTL0 |= ADC12ENC;                      // Enable conversion

    //Usart Setting for Serial traces
    P4SEL |= BIT5 + BIT4;            // Set pins for use with UART: P4.5 = RXD, P4.4=TXD
    P4DIR |= BIT4;                    // set as output (RXD)
    P4DIR &= ~BIT5;                   // set as input (TXD) (not used in this code!)

    UCA1CTL1 |= UCSWRST;                // USCI module in reset mode
    UCA1CTL1 |= UCSSEL_2;               // SMCLK
    UCA1BR0 = 0x68;                     // Low Byte for 9600 Bd (see User's Guide)
    UCA1BR1 = 0x00;                     // High Byte for 9600 Bd
    UCA0MCTL |= UCBRS_1 + UCBRF_0;      // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL0 = 0x00;                    // No parity, LSB first, 8-bit, one stop bit
    UCA1CTL1 &= ~UCSWRST;               // USCI module released for operation

    uart_printf("\ec");

    __enable_interrupt();

    while(1){
        //main program loop, other sensor functions and calculations and logic here

        light_ldr_measurement();

        __delay_cycles(1000000); //1s interval per loop
    }
}

//ADC ISR
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    switch (ADC12IV) {
        case ADC12IV_ADC12IFG3: //iv value is 0x0C; when EOS ADC channel triggers
            adc_vals[0] = ADC12MEM0; adc_vals[1] = ADC12MEM1;
            adc_vals[2] = ADC12MEM2; adc_vals[3] = ADC12MEM3;
            break;
    }
}

//LDR function to detect sky ambient light
void light_ldr_measurement() {
    ADC12CTL0 |= ADC12SC;    //adc conversion

    //measure ldr voltage from volt divider via ADCs
    float v_ldr1 = 3.3*(adc_vals[0]/4095.0); float v_ldr2 = 3.3*(adc_vals[1]/4095.0);
    float v_ldr3 = 3.3*(adc_vals[2]/4095.0); float v_ldr4 = 3.3*(adc_vals[3]/4095.0);

    //determine each ldr resistance then find the average. each is unsigned 16 bit cause ldr resistance can be > 32767 ohms
    uint16_t r_ldr1 = (v_ldr1*volt_dvr_r1s[0])/(3.3-v_ldr1); uint16_t r_ldr2 = (v_ldr2*volt_dvr_r1s[1])/(3.3-v_ldr2);
    uint16_t r_ldr3 = (v_ldr3*volt_dvr_r1s[2])/(3.3-v_ldr3); uint16_t r_ldr4 = (v_ldr4*volt_dvr_r1s[3])/(3.3-v_ldr4);
    uint16_t r_ldr_avg = (uint16_t)( ((uint32_t)r_ldr1 + r_ldr2 + r_ldr3 + r_ldr4) / 4.0 ); //the sum of all 4 resistance can be > 65535. so 32-bits.

    //determine sky ambeint light intensity from r_ldr_avg
    char* ldr_output_str = "";
    if (r_ldr_avg >= 10000) { ldr_output_str = "Night-time darkness"; }
    else if (r_ldr_avg >= 1000 && r_ldr_avg < 10000) { ldr_output_str = "Indoor environment"; }
    else if (r_ldr_avg >= 300 && r_ldr_avg < 1000) { ldr_output_str = "Outdoor no sunlight"; }
    else if (r_ldr_avg >= 100 && r_ldr_avg < 300) { ldr_output_str = "Outdoor moderate sunlight"; }
    else if (r_ldr_avg >= 0 && r_ldr_avg < 100) { ldr_output_str = "Outdoor full sunlight"; }

    uart_printf(ldr_output_str); uart_printf("; ");
    uart_printf("R_LDR_avg:"); integerToUsart(r_ldr_avg); uart_printf(usartValue);
    //uart_printf("R_LDR1:"); integerToUsart(r_ldr1); uart_printf(usartValue);
    //uart_printf(",R_LDR2:"); integerToUsart(r_ldr2); uart_printf(usartValue);
    //uart_printf(",R_LDR3:"); integerToUsart(r_ldr3); uart_printf(usartValue);
    //uart_printf(",R_LDR4:"); integerToUsart(r_ldr4); uart_printf(usartValue);
    uart_printf("\r\n");      // carriage return

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
