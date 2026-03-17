/*
 * ADC.h
 *
 *  Created on: Mar 17, 2026
 *      Author: ameri
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void initADC(void);
uint16_t readADC(uint8_t channel);

#endif
