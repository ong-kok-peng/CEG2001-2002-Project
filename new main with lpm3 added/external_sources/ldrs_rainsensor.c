/*
 * ldrs_rainsensor.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng and Augustine Yeo
 */

#include "ldrs_rainsensor.h"
#include <msp430.h>

const uint16_t ldrVoltDivR1s[4] = {506, 489, 490, 490};
volatile uint16_t ldrADCValues[4] = {0, 0, 0, 0};
volatile uint16_t rainSensorADC = 0;
volatile uint16_t averageLDRResistance = 0;
volatile char adcReadingDone = 0;

void initADCsForLDRs(void) {
    //init ADCs registers for P6.0 to P6.3, and P6.5
    P6SEL |= BIT0 | BIT1 | BIT2 | BIT3 | BIT5;                   // P6.0 thru P6.3 ADC option select, P6.5 as well

    ADC12CTL0 = ADC12SHT02 + ADC12MSC + ADC12ON;          // Sampling time, ADC12 on
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_1;                 // Use sampling timer, with sequence of ADC channels

    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;               // Vr+=AVcc and Vr-=AVss; Select Channel A0
    ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_1;               // select channel A1
    ADC12MCTL2 = ADC12SREF_0 + ADC12INCH_2;               // select channeL A2
    ADC12MCTL3 = ADC12SREF_0 + ADC12INCH_3;               // select channel A3
    ADC12MCTL4 = ADC12SREF_0 + ADC12INCH_5  + ADC12EOS;   // select channel A5 with end of seq

    ADC12IE |= ADC12IE4;                         // Enable ADC interrupt for last ADC channel as EOS
    ADC12CTL0 |= ADC12ENC;                      // Enable conversion
}

void beginReadADCs(void) {
    adcReadingDone = 0;
    while (ADC12CTL1 & ADC12BUSY) {}
    ADC12CTL0 &= ~ADC12SC;
    ADC12IFG = 0;
    ADC12CTL0 |= ADC12SC;    //adc conversion
}

void readLDRsResistance(void) {
    //measure ldr voltage from volt divider via ADCs
    float v_ldr1 = 3.3*(ldrADCValues[0] / 4095.0);
    float v_ldr2 = 3.3*(ldrADCValues[1] / 4095.0);
    float v_ldr3 = 3.3*(ldrADCValues[2] / 4095.0);
    float v_ldr4 = 3.3*(ldrADCValues[3] / 4095.0);

    //determine each ldr resistance then find the average. each is unsigned 16 bit cause ldr resistance can be > 32767 ohms
    uint16_t r_ldr1 = (v_ldr1 * ldrVoltDivR1s[0]) / (3.3 - v_ldr1);
    uint16_t r_ldr2 = (v_ldr2 * ldrVoltDivR1s[1]) / (3.3 - v_ldr2);
    uint16_t r_ldr3 = (v_ldr3 * ldrVoltDivR1s[2]) / (3.3 - v_ldr3);
    uint16_t r_ldr4 = (v_ldr4 * ldrVoltDivR1s[3]) / (3.3 - v_ldr4);
    averageLDRResistance = (uint16_t)( ((uint32_t)r_ldr1 + r_ldr2 + r_ldr3 + r_ldr4) / 4.0 ); //the sum of all 4 resistance can be > 65535. so 32-bits.
}


/*---------------ADC ISR----------------*/
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    switch (ADC12IV) {
       case ADC12IV_ADC12IFG4: //iv value is 0x0C; when EOS ADC channel triggers
           ldrADCValues[0] = ADC12MEM0;
           ldrADCValues[1] = ADC12MEM1;
           ldrADCValues[2] = ADC12MEM2;
           ldrADCValues[3] = ADC12MEM3;
           rainSensorADC = ADC12MEM4;

           adcReadingDone = 1;
           __bic_SR_register_on_exit(LPM0_bits);
           break;
    }
}
