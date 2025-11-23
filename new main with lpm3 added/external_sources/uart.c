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
    P4SEL |= BIT4 + BIT5;           // P4.4 TX, P4.5 RX
    UCA1CTL1 |= UCSWRST;            // Put USCI in reset
    UCA1CTL1 |= UCSSEL_2;           // Use SMCLK (1 MHz)
    UCA1BR0 = 104;                  // 1MHz / 9600
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;             // Modulation
    UCA1CTL1 &= ~UCSWRST;           // Initialize USCI

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
