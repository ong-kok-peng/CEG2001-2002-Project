/*
 * dht22.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Yeo Zhi Wei
 */

//TAKE NOTE MSP430F5529 MUST CONFIGURE FOR 8MHZ SMCLK AND MCLK FIRST BEFORE INCLUDING DHT22.H!!

#include "dht22.h"
#include <msp430.h>

volatile char dht22_intervalUp = 0;
volatile uint8_t  dht22_risingDetected = 0;
volatile uint16_t dht22_prevRiseTime = 0;

volatile char dht22_bits[40];
volatile uint8_t dht22_bitcount = 0;
volatile char dht22_dataCollected = 0;

const uint8_t dht22_beginEdgeCount = 2;
volatile uint8_t dht22_edgeCount = 0;
const uint16_t dht22_readTimeOut = 10000;   //waiting timeout 10ms in us
volatile char dht22_timedOut = 0;

volatile uint16_t humidity = 0;             //multiplied by 10 int
volatile uint16_t temperature = 0;          //multiplied by 10 int

void initDHT22(void) {
    //BEFORE CALLING THIS, ENSURE TA0CTL REGISTER IS SET!!
    TA0CCTL2 = CM_3 | CCIS_0 | SCS | CAP;
}

void beginDHT22Reading(void) {
    //reset all flags and the 40-bit data
    //THE DHT22 MUST CONNECT TO P1.3!!
    dht22_dataCollected = 0; dht22_bitcount = 0; dht22_edgeCount = 0;
    int a; for (a=0; a < 40; a++) { dht22_bits[a] = 0; }

    P1SEL &= ~BIT3; //enable P1.3 as gpio
    P1DIR |= BIT3; //init P1.3 as output

    P1OUT |= BIT3; //init P1.3 high
    __delay_cycles(800); //delay for some microseconds
    P1OUT &= ~BIT3; //pull P1.3 low
    __delay_cycles(24000); //delay for 3ms
    P1OUT |= BIT3; //pull P1.3 high again
    __delay_cycles(400); //delay for 50us

    P1DIR &= ~BIT3; //set now P1.3 as input
    P1SEL |= BIT3; //disable P1.3 as gpio

    TA0CCTL2 &= ~CCIFG;   // clear stale flag for capture ta0ccr2
    TA0CCTL2 |= CCIE;     // now enable IRQ for capture ta0ccr2
}

void readDHT22Reading(void) {
    uint16_t dht22_startTime = TA0R;

    while (!dht22_dataCollected) {
        //wait for dht22 data to collect until it times out
        if ((uint16_t)(TA0R - dht22_startTime) >= dht22_readTimeOut) { dht22_timedOut = 1; break; }
    }
    if (dht22_dataCollected && !dht22_timedOut){
        //take humidity value from dht22_bits[0] to [15]
        int b;
        for (b = 0; b < 16; ++b) {
            humidity = (humidity << 1) | (dht22_bits[0 + b] & 1);
        }

        //take temperature value from dht22_bits[16] to [31]
        for (b = 16; b < 32; ++b) {
            temperature = (temperature << 1) | (dht22_bits[0 + b] & 1);
        }
    }
}

//timerA0 capture mode ISR
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void) {
    switch (TA0IV) {
    case TA0IV_TACCR2: {
        uint16_t dht22_timestamp = TA0CCR2;

        // If we missed an edge, resync this bit
        if (TA0CCTL2 & COV) { TA0CCTL2 &= ~COV; dht22_risingDetected = 0; }

        // Count edges and skip the 2 presence edges
        dht22_edgeCount++;
        if (dht22_edgeCount <= dht22_beginEdgeCount) { dht22_risingDetected = 0; break; }

        // Read pin level: high => we just captured a rising edge; low => falling
        uint8_t dht22_riseOrFall = (P1IN & BIT3) ? 1 : 0;

        if (dht22_riseOrFall) {
            // RISING: mark start of HIGH pulse
            dht22_prevRiseTime = dht22_timestamp;
            dht22_risingDetected = 1;
        } else {
            // FALLING: HIGH pulse ended → measure width
            if (dht22_risingDetected && dht22_bitcount < 40) {
                uint16_t dt = (uint16_t)(dht22_timestamp - dht22_prevRiseTime);

                // small sanity window to drop glitches
                if (dt >= 10 && dt <= 200) {
                    dht22_bits[dht22_bitcount++] = (dt >= 50) ? 1 : 0; // threshold ~50 µs
                    if (dht22_bitcount == 40) {
                        dht22_dataCollected = 1;
                        TA0CCTL2 &= ~CCIE;     // done
                    }
                }
            }
            dht22_risingDetected = 0;
        }
    } break;
    default: break;
    }
}
