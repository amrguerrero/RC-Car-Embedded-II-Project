/*
 * ADC.c
 *
 *  Created on: Mar 17, 2026
 *      Author: ameri
 */




#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "adc.h"

void initADC(void)
{
    // Enable ADC0 and Port E
    SYSCTL_RCGCADC_R |= 1;
    SYSCTL_RCGCGPIO_R |= 0x10;
    _delay_cycles(3);

    // Configure PE0–PE3 as analog
    GPIO_PORTE_DIR_R &= ~0x0F;
    GPIO_PORTE_AFSEL_R |= 0x0F;
    GPIO_PORTE_DEN_R &= ~0x0F;
    GPIO_PORTE_AMSEL_R |= 0x0F;

    // Configure ADC0 SS3 (single sample)
    ADC0_ACTSS_R &= ~8;              // disable SS3
    ADC0_EMUX_R &= ~0xF000;          // software trigger
    ADC0_SSMUX3_R = 0;               // default AIN0
    ADC0_SSCTL3_R = 6;               // END0 + IE0
    ADC0_ACTSS_R |= 8;               // enable SS3
}

uint16_t readADC(uint8_t channel)
{
    // Select channel (AIN0–AIN3)
    ADC0_SSMUX3_R = channel;

    ADC0_PSSI_R = 8;                 // start conversion
    while ((ADC0_RIS_R & 8) == 0);   // wait
    uint16_t result = ADC0_SSFIFO3_R & 0xFFF;
    ADC0_ISC_R = 8;                  // clear flag

    return result;
}
