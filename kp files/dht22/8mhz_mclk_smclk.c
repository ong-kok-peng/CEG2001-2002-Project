//****************************************************************
//8mhz MCLK SMCLK demo
//__delay_cycles() must multiply value by 8!!
//when using timer, makesure prescaler is /8!!
#include <msp430.h>

void msp430f5529_mclk_smclk_8mhz(void);

int main(void)
{
  volatile unsigned int i;

  WDTCTL = WDTPW+WDTHOLD;                   // Stop WDT
  P1DIR |= BIT1;                            // P1.1 output

  msp430f5529_mclk_smclk_8mhz();
	
  while(1)
  {
    P1OUT ^= BIT0;                          // Toggle P1.0
    __delay_cycles(600000);                 // Delay
  }
}

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
