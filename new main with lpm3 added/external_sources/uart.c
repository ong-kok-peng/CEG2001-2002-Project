/*
 * uart.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng
 */

#include "uart.h"
#include <msp430.h>

volatile uint8_t  uartArduino_buf[64];
volatile uint16_t uartArduino_buflen  = 0;
volatile uint8_t  uartArduinoRXDone = 0;

volatile char uartMsgDebug[64];
volatile char uartMsgArduino[64];

void initUARTDebug(void) {
    P4SEL |= BIT4 + BIT5;           // P4.4 TX, P4.5 RX
    UCA1CTL1 |= UCSWRST;            // Put USCI in reset
    UCA1CTL1 |= UCSSEL_2;           // Use SMCLK (1 MHz)
    UCA1BR0 = 104;                  // 1MHz / 9600
    UCA1BR1 = 0;
    UCA1MCTL = UCBRS_1;             // Modulation
    UCA1CTL1 &= ~UCSWRST;           // Initialize USCI
}

void initUARTArduino(void) {
    P3SEL |= BIT3 + BIT4;           // P3.3 TX, P3.4 RX
    UCA0CTL1 |= UCSWRST;            // Put USCI in reset
    UCA0CTL1 |= UCSSEL_2;           // Use SMCLK (1 MHz)
    UCA0BR0 = 104;                  // 1MHz / 9600
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS_1;             // Modulation
    UCA0IE   = UCRXIE;
    UCA0CTL1 &= ~UCSWRST;           // Initialize USCI
}

void uart_printDebug(char *str)
{
    while (*str)
    {
        while (!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = *str++;
    }
}

void uart_printArduino(char *str)
{
    while (*str)
    {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = *str++;
    }
}

//ISR for UCA0 RX
#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    P1OUT ^= BIT0;

    switch (UCA0IV) {
        case 2: { // UCRXIFG
            uint8_t b = UCA0RXBUF;

            if (uartArduino_buflen < 63) {     // leave room for '\0'
                uartArduino_buf[uartArduino_buflen++] = b;
            }

            uartArduinoRXDone = 1;
        } break;

        default: break;
    }
}
