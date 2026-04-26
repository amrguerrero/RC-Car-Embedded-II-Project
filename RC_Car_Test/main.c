//==============================================================
// TM4C123GXL CLEAN CONTROL SYSTEM (UPDATED PIN MAP)
// Servo + Motor + Ultrasonic
//==============================================================

#include <stdint.h>
#include <stdbool.h>

#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "clock.h"

//==============================================================
// PIN DEFINITIONS (UPDATED)
//==============================================================

// Servo PWM
#define SERVO_PORT PORTF
#define SERVO_PIN  1   // PF1 M1PWM5

// Motor PWM (Enable)
#define MOTOR_PWM_PORT PORTF
#define MOTOR_PWM_PIN  3   // PF3 M1PWM7

// Motor Direction (SN754410NE)
#define A1_PORT PORTD
#define A1_PIN  0
#define A2_PORT PORTD
#define A2_PIN  1

// Ultrasonic
#define TRIG_PORT PORTC
#define TRIG_PIN  4
#define ECHO_PORT PORTC
#define ECHO_PIN  5

//==============================================================
// PWM SETTINGS
//==============================================================
#define PWM_LOAD 12499   // 50Hz

#define SERVO_MIN  625
#define SERVO_MAX  1250

volatile uint16_t motorCurrentDuty = 0;
volatile uint16_t motorTargetDuty  = 0;

//==============================================================
// PWM INIT
//==============================================================
void initPwm(void)
{

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // Port F
    waitMicrosecond(5);

    GPIO_PORTF_DIR_R |= (1 << 1) | (1 << 3);
    GPIO_PORTF_DEN_R |= (1 << 1) | (1 << 3);
    GPIO_PORTF_AFSEL_R |= (1 << 1) | (1 << 3);
    GPIO_PORTF_PCTL_R =
        (GPIO_PORTF_PCTL_R & 0xFFFF00FF) |
        (GPIO_PCTL_PF1_M1PWM5 | GPIO_PCTL_PF3_M1PWM7);

    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // Port F
    waitMicrosecond(5);

    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M)
                 | SYSCTL_RCC_PWMDIV_64;

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;
    SYSCTL_SRPWM_R = 0;

    //========================
    // Servo PF1 M1PWM5 (PWM1_2)
    //========================
    PWM1_2_CTL_R = 0;
    PWM1_2_LOAD_R = PWM_LOAD;
    PWM1_2_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;

    //========================
    // Motor PF3 M1PWM7 (PWM1_3)
    //========================
    PWM1_3_CTL_R = 0;
    PWM1_3_LOAD_R = PWM_LOAD;
    PWM1_3_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;

    PWM1_2_CTL_R = PWM_1_CTL_ENABLE;
    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;

    PWM1_ENABLE_R |= PWM_ENABLE_PWM5EN | PWM_ENABLE_PWM7EN;
}

//==============================================================
// SERVO (PF1)
//==============================================================
void initServo(void)
{
    setPinAuxFunction(SERVO_PORT, SERVO_PIN, GPIO_PCTL_PF1_M1PWM5);
    selectPinPushPullOutput(SERVO_PORT, SERVO_PIN);
}

void setServoAngle(uint16_t angle)
{
    if (angle > 180) angle = 180;

    uint16_t pulse =
        SERVO_MIN + ((SERVO_MAX - SERVO_MIN) * angle) / 180;

    PWM1_2_CMPB_R = pulse;
}

//==============================================================
// MOTOR (SN754410NE)
//==============================================================
void initMotor(void)
{
    selectPinPushPullOutput(A1_PORT, A1_PIN);
    selectPinPushPullOutput(A2_PORT, A2_PIN);

    setPinAuxFunction(MOTOR_PWM_PORT, MOTOR_PWM_PIN,
                      GPIO_PCTL_PF3_M1PWM7);
}


// ramp function (smooth acceleration)
void updateMotorRamp(void)
{
    const uint16_t step = 1;

    if (motorCurrentDuty < motorTargetDuty)
        motorCurrentDuty += step;
    else if (motorCurrentDuty > motorTargetDuty)
        motorCurrentDuty -= step;

    PWM1_3_CMPB_R = (PWM_LOAD * motorCurrentDuty) / 100;
}

void setMotor(int8_t speed)
{
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    // STOP
    if (speed == 0)
    {
        setPinValue(A1_PORT, A1_PIN, false);
        setPinValue(A2_PORT, A2_PIN, false);
        motorTargetDuty = 0;
        return;
    }

    // Direction
    if (speed > 0)
    {
        setPinValue(A1_PORT, A1_PIN, true);
        setPinValue(A2_PORT, A2_PIN, false);
    }
    else
    {
        setPinValue(A1_PORT, A1_PIN, false);
        setPinValue(A2_PORT, A2_PIN, true);
        speed = -speed;
    }

    // Map 0–100 → 75–100
    motorTargetDuty = 75 + ((speed * 25) / 100);
}

//==============================================================
// ULTRASONIC (PC4 / PC5)
//==============================================================
void initUltrasonic(void)
{
    selectPinDigitalInput(ECHO_PORT, ECHO_PIN);
    selectPinPushPullOutput(TRIG_PORT, TRIG_PIN);
}

uint32_t getDistanceCm(void)
{
    uint32_t time = 0;

    setPinValue(TRIG_PORT, TRIG_PIN, false);
    waitMicrosecond(2);

    setPinValue(TRIG_PORT, TRIG_PIN, true);
    waitMicrosecond(10);
    setPinValue(TRIG_PORT, TRIG_PIN, false);

    uint32_t timeout = 30000;

    while (!getPinValue(ECHO_PORT, ECHO_PIN))
    {
        if (--timeout == 0)
            return 999; // out of range / no echo
    }

    timeout = 30000;
    while (getPinValue(ECHO_PORT, ECHO_PIN))
    {
        if (--timeout == 0)
            break;
        waitMicrosecond(1);
        time++;
    }

    return time / 58;
}

//==============================================================
// MAIN
//==============================================================
int main(void)
{
    initSystemClockTo40Mhz();

    // Enable all GPIO ports you use
    enablePort(PORTF);   // PF1, PF3
    enablePort(PORTD);   // PD0, PD1
    enablePort(PORTC);   // PC4, PC5

    initPwm();
    initServo();
    initMotor();
    initUltrasonic();

    while (1)
    {
        uint32_t dist = getDistanceCm();

        if (dist < 15)
        {
            setMotor(0);
            setServoAngle(0);
        }
        else
        {
            setMotor(60);
            setServoAngle(90);
        }

        updateMotorRamp();
        waitMicrosecond(5000);
    }
}
