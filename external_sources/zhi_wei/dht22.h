/*
 * dht22.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Yeo Zhi Wei
 */

#include <stdint.h>

#ifndef DHT22_H
#define DHT22_H

void initDHT22(void);
void beginDHT22Reading(void);
void readDHT22Reading(void);

extern volatile char dht22_intervalUp;
extern volatile uint8_t  dht22_risingDetected;
extern volatile uint16_t dht22_prevRiseTime;

extern volatile char dht22_bits[40];
extern volatile uint8_t dht22_bitcount;
extern volatile char dht22_dataCollected;

extern volatile uint8_t dht22_edgeCount;
extern volatile char dht22_timedOut;

extern volatile uint16_t humidity;
extern volatile uint16_t temperature;

#endif
