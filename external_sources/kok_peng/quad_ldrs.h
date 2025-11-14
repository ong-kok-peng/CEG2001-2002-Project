/*
 * quad_ldrs.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Ong Kok Peng
 */

#include <stdint.h>

#ifndef QUAD_LDRS_H
#define QUAD_LDRS_H

void initADCsForLDRs(void);
void readLDRsResistance(void);

extern volatile uint16_t ldrADCValues[4];
extern volatile uint16_t averageLDRResistance;

#endif
