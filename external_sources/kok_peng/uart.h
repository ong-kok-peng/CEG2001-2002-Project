/*
 * uart.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng
 */

#ifndef UART_H
#define UART_H

void initUART(void);
void uart_printf(const char* data);
void integerToUsart(unsigned int integer);

extern char usartValue[6];

#endif
