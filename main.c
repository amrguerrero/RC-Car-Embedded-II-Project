

/**
 * main.c
 */

/*
 HARDWARE PIN OUTPUTS:

 NRF24LO1
 --------------
 VCC        3.3V
 GND
 CE         PA6
 CSN        PA3
 SCK        PA2
 MOSI       PA5
 MISO       PA4
 IRQ        Unconnected (not needed for TX)

 Joysticks
 -----------
 J1 VRx     PE3
 J1 VRy     PE2
 J2 VRx     PE1
 J2 VRy     PE0
 VCC        3.3V
 GND
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "CTI.h"
#include "clock80.h"
#include "ADC.h"
#include "spi0.h"
#include "NRF.h"

// ADC channel numbers
#define AIN0  0   // PE3 - J1 VRx (steer X)
#define AIN1  1   // PE2 - J1 VRy (steer Y)
#define AIN2  2   // PE1 - J2 VRy (throttle)
#define AIN3  3   // PE0 - J2 VRx (trim, optional)

#define DEADBAND 5

uint8_t TxAddress[] = { 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };

// J1 - Movement Joystick
#define MOVE_X_PIN PE3      // AIN0
#define MOVE_Y_PIN PE2      // AIN1

// J2 - Speed Joystick
#define SPEED_Y_PIN PE1     // AIN2
#define SPEED_X_PIN PE0     // AIN3 //optional/trim

uint16_t throttleCenter, steerXCenter, steerYCenter;

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

uint16_t invertADC(uint16_t raw)
{
    return 4095 - raw;
}

void calibrateJoysticks(void)
{
    putsUart0("Calibrating... release joysticks\r\n");
    _delay_cycles(160000000);

    uint32_t tSum = 0, xSum = 0, ySum = 0;
    uint8_t i;
    for (i = 0; i < 32; i++)
    {
        tSum += invertADC(readADC(AIN1));  // J1 VRy → throttle
        xSum += invertADC(readADC(AIN2));  // J2 VRx → steerX
        ySum += invertADC(readADC(AIN3));  // J2 VRy → steerY
        _delay_cycles(800000);
    }
    throttleCenter = tSum / 32;
    steerXCenter   = xSum / 32;
    steerYCenter   = ySum / 32;

    putsUart0("Centers - T:");
    putintUart0(throttleCenter);
    putsUart0(" X:");
    putintUart0(steerXCenter);
    putsUart0(" Y:");
    putintUart0(steerYCenter);
    putsUart0("\r\n");
    putsUart0("Calibration done\r\n");
}

int8_t mapJoystickCalibrated(uint16_t raw, uint16_t center)
{
    int32_t shifted = (int32_t)raw - center;
    int32_t range = (shifted >= 0) ? (4095 - center) : center;
    return (int8_t)(shifted * 100 / range);
}

ControlPacket readControllerInputs(void)
{
    ControlPacket pkt;
    pkt.throttle = applyDeadband(
        mapJoystickCalibrated(invertADC(readADC(AIN1)), throttleCenter), DEADBAND);
    pkt.steerX = applyDeadband(
        mapJoystickCalibrated(invertADC(readADC(AIN2)), steerXCenter), DEADBAND);
    pkt.steerY = applyDeadband(
        mapJoystickCalibrated(invertADC(readADC(AIN3)), steerYCenter), DEADBAND);
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

    // NRF control pins



}

// Pack 3 int8 values into uint32 to match teammate's transmit format
uint32_t packPacket(ControlPacket *pkt)
{
    uint8_t throttleByte = (uint8_t)(pkt->throttle + 100);
    uint8_t steerByte    = (uint8_t)(pkt->steerX   + 100);
    uint8_t steerYByte   = (uint8_t)(pkt->steerY   + 100);

    return ((uint32_t)throttleByte << 16) |
           ((uint32_t)steerByte    << 8) |
           ((uint32_t)steerYByte   << 0);
}

void initHardware(void)
{
    initSystemClockTo80Mhz();
    initUart0();
    setUart0BaudRate(115200, 80000000);
    initGPIO();
    initADC();
    calibrateJoysticks();



    // NRF24L01 SPI setup
    initSpi0(USE_SSI0_RX);
    setSpi0BaudRate(8000000, 80000000);  // 1 MHz, note your clock is 80MHz not 40MHz
    setSpi0Mode(0, 0);                   // Mode 0

    NRF24_Init();
    NRF24_TxMode(TxAddress, 80);
}

int main(void)
{
    initHardware();

    while(1)
    {
        ControlPacket pkt = readControllerInputs();

//        putsUart0("T:");
//        putintUart0((uint32_t)(pkt.throttle + 100));
//        putsUart0(" X:");
//        putintUart0((uint32_t)(pkt.steerX + 100));
//        putsUart0(" Y:");
//        putintUart0((uint32_t)(pkt.steerY + 100));
//        putsUart0("\r\n");

        uint32_t txData = packPacket(&pkt);
//        putsUart0("TX -> T:");
//        putintUart0((txData >> 24) & 0xFF);
//        putsUart0(" X:");
//        putintUart0((txData >> 16) & 0xFF);
//        putsUart0(" Y:");
//        putintUart0((txData >>  8) & 0xFF);
//        putsUart0("\r\n");

        NRF24_Transmit(txData);

        putsUart0("RAW AIN1:");
        putintUart0(readADC(AIN1));
        putsUart0("\r\n");
        _delay_cycles(800000);
    }
}
