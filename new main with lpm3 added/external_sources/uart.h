/*
 * uart.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng
 */

#include <stdint.h>

#ifndef UART_H
#define UART_H

void initUARTDebug(void);
void uart_printDebug(char* str);

void initUARTArduino(void);
void uart_printArduino(char* str);

extern volatile uint8_t  uartArduino_buf[64];
extern volatile uint16_t uartArduino_buflen;
extern volatile uint8_t  uartArduinoRXDone;

extern volatile char uartMsgDebug[64];
extern volatile char uartMsgArduino[64];

#endif
