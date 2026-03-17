/*
 * PWM.h
 *
 *  Created on: Mar 17, 2026
 *      Author: ameri
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>
#include <stdbool.h>

// =====================
// Initialization
// =====================

// Initialize PWM0 on PB6 (M0PWM0)
// Configured for ~1 kHz at 40 MHz system clock
void initPWM0(void);

// =====================
// Duty Cycle Control
// =====================

// Set duty cycle (0–100%)
void setPWM0Duty(uint8_t dutyPercent);

// =====================
// Utility (Optional)
// =====================

// Get current duty cycle (if you track it in .c file)
uint8_t getPWM0Duty(void);

// =====================
// Future Expansion (RC Car)
// =====================

// Generic motor-style interface (you can implement later)
void setMotorSpeed(uint8_t dutyPercent);

// Stop PWM output (0% duty)
void stopPWM0(void);

#endif /* PWM_H_ */
