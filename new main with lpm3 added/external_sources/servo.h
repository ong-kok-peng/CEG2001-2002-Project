/*
 * servo.h
 *
 *  Created on: Nov 17, 2025
 *      Author: Ong Kok Peng
 */

#include <stdint.h>

#ifndef SERVO_H
#define SERVO_H

void init_servo(void);
void set_servo(void);

extern volatile char servoActivated;

#endif
