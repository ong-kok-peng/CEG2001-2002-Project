/*
 * uart.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng
 */

//TAKE NOTE MSP430F5529 MUST CONFIGURE FOR 8MHZ SMCLK AND MCLK FIRST BEFORE INCLUDING UART.H!!

#include "uart.h"
#include <msp430.h>

char usartValue[6] = { 0x20, 0x20, 0x20, 0x20, 0x20,'\0' };

void initUART(void) {
    P4SEL |= BIT5 + BIT4;                       // Set pins for use with UART: P4.5 = RXD, P4.4=TXD
    P4DIR |= BIT4;                              // set as output (RXD)
    P4DIR &= ~BIT5;                             // set as input (TXD) (not used in this code!)

    UCA1CTL1 |= UCSWRST;                        // USCI module in reset mode
    UCA1CTL1 |= UCSSEL_2;                       // SMCLK at 8Mhz!!
    UCA1BR0 = 0x04;                             // Low Byte for 115200 Bd (see User's Guide)
    UCA1BR1 = 0x00;                             // High Byte for 115200 Bd
    UCA1MCTL |= UCBRS_4 | UCBRF_7 | UCOS16;     // Modulation UCBRSx=4, UCBRFx=7, UCOS16 = 1;
    UCA1CTL0 = 0x00;                            // No parity, LSB first, 8-bit, one stop bit
    UCA1CTL1 &= ~UCSWRST;                       // USCI module released for operation

}

void uart_printf(const char* data) {
    volatile char i = 0;
    while (data[i] != '\0') {                   // Check for end of string '\0'
        UCA1TXBUF = data[i];                    // Put character to buffer
        while (UCA1STAT & UCBUSY);              // Wait until char is sent
        i++;
    }
}

void integerToUsart(unsigned int integer) {
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
