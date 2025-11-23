#include <msp430.h> 
#include <stdint.h>
#include <external_sources/kok_peng/ldrs_rainsensor.h>
#include "./external_sources/kok_peng/uart.h"
//#include "./external_sources/kok_peng/servo.h"

void msp430f5529_smclk_1mhz(void);
void set_capturecompare_timer(void);
void set_3seconds_stopwatch(void);

volatile char do_work = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	msp430f5529_smclk_1mhz(); //MUST RUN FIRST!!
	set_capturecompare_timer();
	set_3seconds_stopwatch();

	initUART();
	initADCsForLDRs();
	//init_servo();

	__enable_interrupt();

	while (1) {
      __bis_SR_register(LPM3_bits | GIE);   //put to LPM3 mode until every 5 seconds later from TA1
      __no_operation();

      if (do_work) {
        do_work = 0;
        beginReadADCs();

        // --- Inner loop: LPM0 until both done ---
        while (!(adcReadingDone)) {
          __bis_SR_register(LPM0_bits | GIE);  // turn SMCLK on
          __no_operation();
        }

        readLDRsResistance();
        //determine sky ambient light intensity from averageLDRResistance
        char* ldr_output_str = "";
        if (averageLDRResistance >= 10000) { ldr_output_str = "Night-time darkness"; }
        else if (averageLDRResistance >= 1000 && averageLDRResistance < 10000) { ldr_output_str = "Indoor environment"; }
        else if (averageLDRResistance >= 300 && averageLDRResistance < 1000) { ldr_output_str = "Outdoor no sunlight"; }
        else if (averageLDRResistance >= 100 && averageLDRResistance < 300) { ldr_output_str = "Outdoor moderate sunlight"; }
        else if (averageLDRResistance >= 0 && averageLDRResistance < 100) { ldr_output_str = "Outdoor full sunlight"; }

        uart_printf(ldr_output_str); uart_printf("; ");
        uart_printf("R_LDR_avg:"); integerToUsart(averageLDRResistance); uart_printf(usartValue);
        uart_printf(";Rainsensor ADC: "); integerToUsart(rainSensorADC); uart_printf(usartValue);
        uart_printf("\r\n");

      }

	}

}

//CONFIGURE TIMER_A0 AS CAPTURE/COMPARE TIMER
void set_capturecompare_timer(void) {
    TA0CTL = TASSEL_2 | ID_0 | MC_1 | TACLR;        // SMCLK, /1, up-count mode till ta0ccr0, clear
    TA0CCR0 = 20000;                                // capture & compare for 20ms period max (50Hz for servo motor)
}

//CONFIGURE TIMER_A1 AS SECONDS INTERVAL
void set_3seconds_stopwatch(void) {
    TA1CTL = TASSEL_1 | ID_3 | MC_1 | TACLR;        // ACLK, /8, count up to TA1CCR0, clear
    TA1CCR0 = 3*4096;                                 // 1 tick is 0.244ms, so 1s = 4096 ticks
    TA1CCTL0 = CCIE;
}

//TIMER_A1 ISR
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void) {
    do_work = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}

//CONFIGURE MSP430F5529 TO RUN SMCLK AT 1MHZ!!
void msp430f5529_smclk_1mhz(void) {
    UCSCTL3 = SELREF__REFOCLK;          // FLL reference = REFO
    UCSCTL4 |= SELA__REFOCLK;           // ACLK = REFO
    __bis_SR_register(SCG0);            // Disable FLL control loop
    UCSCTL0 = 0x0000;                   // Reset DCO and modulation
    UCSCTL1 = DCORSEL_2;                // DCO range ~1MHz
    UCSCTL2 = FLLD_1 | 30;              // (N + 1) * FLLRef = 32768 * 31 / 1 = ~1.013 MHz
    __bic_SR_register(SCG0);            // Enable FLL control loop
    __delay_cycles(250000);             // Wait for DCO to settle
}
