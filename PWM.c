#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "pwm.h"

// =====================
// Static Globals
// =====================
static uint32_t pwmLoad = 0;
static uint8_t currentDuty = 0;

// =====================
// Initialize PWM0 (PB6)
// =====================
void initPWM0(void)
{
    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;   // Port B
    SYSCTL_RCGCPWM_R  |= SYSCTL_RCGCPWM_R0;    // PWM0
    _delay_cycles(3);

    // Configure PB6 as M0PWM0
    GPIO_PORTB_DIR_R   |= (1 << 6);
    GPIO_PORTB_DEN_R   |= (1 << 6);
    GPIO_PORTB_AFSEL_R |= (1 << 6);

    // Set PCTL for PB6 → M0PWM0 (function 4)
    GPIO_PORTB_PCTL_R =
        (GPIO_PORTB_PCTL_R & ~(0xF << (6 * 4))) | (4 << (6 * 4));

    // PWM clock = system clock / 64
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M) | SYSCTL_RCC_PWMDIV_64;

    // Disable generator during config
    PWM0_0_CTL_R = 0;

    // HIGH at LOAD, LOW at CMPA
    PWM0_0_GENA_R = PWM_0_GENA_ACTLOAD_ONE | PWM_0_GENA_ACTCMPAD_ZERO;

    // 1 kHz PWM: 40 MHz / 64 = 625 kHz → 625 ticks
    pwmLoad = 625;
    PWM0_0_LOAD_R = pwmLoad - 1;

    // Default duty = 0%
    setPWM0Duty(0);

    // Enable PWM generator
    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;
    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN;
}

// =====================
// Set Duty Cycle
// =====================
void setPWM0Duty(uint8_t dutyPercent)
{
    if (dutyPercent > 100)
        dutyPercent = 100;

    currentDuty = dutyPercent;

    uint32_t cmp = (pwmLoad * dutyPercent) / 100;

    if (cmp == 0)
        cmp = 1; // avoid full-off glitch

    PWM0_0_CMPA_R = cmp - 1;
}

// =====================
// Get Duty Cycle
// =====================
uint8_t getPWM0Duty(void)
{
    return currentDuty;
}

// =====================
// Stop PWM
// =====================
void stopPWM0(void)
{
    setPWM0Duty(0);
}

// =====================
// Motor Wrapper (Future Use)
// =====================
void setMotorSpeed(uint8_t dutyPercent)
{
    setPWM0Duty(dutyPercent);
}
