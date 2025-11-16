/*
 * servo.c
 *
 *  Created on: Nov 17, 2025
 *      Author: Ong Kok Peng
 */

#include "servo.h"
#include <msp430.h>

const uint16_t minPWMWidth = 1000;          //pwm pulse width in us for servo at 0 deg
const uint16_t maxPWMWidth = 2000;          //pwm pulse width in us for servo at 180 deg
volatile char servoActivated = 0;           //activate or deactivate servo (0 is off, 1 is on)

void init_servo(void) {
    //THE SERVO MOTOR MUST CONNECT TO P1.4!!
    TA0CCTL3 = OUTMOD_7;        //set PWM as reset/set
    TA0CCR3 = minPWMWidth;      //set PWM initial pulse width for servo at 0 deg

    P1DIR |= BIT4;              //set P1.4 as output
    P1SEL |= BIT4;              //set P1.4 as peripheral mode, non gpio
}

void set_servo(void) {
    if (servoActivated) { TA0CCR3 = maxPWMWidth; }
    else { TA0CCR3 = minPWMWidth; }
}
