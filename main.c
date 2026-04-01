

/**
 * main.c
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "CTI.h"
#include "clock80.h"
#include "PWM.h"
#include "ADC.h"


// ADC channel numbers
#define AIN0  0   // PE3 - J1 VRx (steer X)
#define AIN1  1   // PE2 - J1 VRy (steer Y)
#define AIN2  2   // PE1 - J2 VRy (throttle)
#define AIN3  3   // PE0 - J2 VRx (trim, optional)

#define DEADBAND 5

// J1 - Movement Joystick
#define MOVE_X_PIN PE3      // AIN0
#define MOVE_Y_PIN PE2      // AIN1

// J2 - Speed Joystick
#define SPEED_Y_PIN PE1     // AIN2
#define SPEED_X_PIN PE0     // AIN3 //optional/trim



typedef struct
{
    int8_t throttle;    // -100 to 100 (reverse to full forward)
    int8_t steerX;      // -100 t0 100 (full left to full right)
    int8_t steerY;      // -100 to 100 (optional, forward/back on move stick
}
ControlPacket;

// Map raw 12-bit ADC (0-4095) to signed range
int8_t mapJoystick(uint16_t raw)
{
    return (int8_t)((raw - 2048) * 100 / 2048);
}


int8_t applyDeadband(int8_t value, uint8_t  threshold)
{
    if (abs(value) < threshold)
        return 0;
    return value;
}

ControlPacket readControllerInputs(void)
{
    ControlPacket pkt;
    pkt.throttle = mapJoystick(readADC(AIN2));  // J2 VRy
    pkt.steerX = mapJoystick(readADC(AIN0));    // J1 VRx
    pkt.steerY = mapJoystick(readADC(AIN1));    // J1 VRy
    return pkt;
}

void initGPIO(void)
{
    SYSCTL_RCGCGPIO_R |= 0x3F; // Enable Port A-F
    _delay_cycles(3);

    // Motor Pins (HOLD)
    GPIO_PORTB_DIR_R |= 0xFF & ~(1 << 6);
    GPIO_PORTB_DEN_R |= 0xFF & ~(1 << 6);

    // LEDS
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;

}


void initHardware(void)
{
    initSystemClockTo80Mhz();
    initUart0();
    setUart0BaudRate(115200, 80000000);

    initGPIO();
    initPWM0();
    initADC();

}

int main(void)
{
    USER_DATA data;
    initHardware();

    while(1)
    {
        ControlPacket pkt = readControllerInputs();

        // Debug print over UART
        putsUart0("T:");
        putintUart0((uint32_t)(pkt.throttle + 100)); // shift for unsigned display
        putsUart0(" X:");
        putintUart0((uint32_t)(pkt.steerX + 100));
        putsUart0(" Y:");
        putintUart0((uint32_t)(pkt.steerY + 100));
        putsUart0("\r\n");

        // TODO: replace with NRF24L01 transmit once SPI is ready
        // nrfTransmit(&pkt, sizeof(pkt));

        _delay_cycles(800000); // ~10ms at 80MHz, simple polling delay
    }
}
