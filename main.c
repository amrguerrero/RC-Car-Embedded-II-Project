

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

void calibrateJoysticks(void)
{
    putsUart0("Calibrating... release joysticks\r\n");
    _delay_cycles(80000000);    // 1 sec delay

    uint32_t tSum = 0, xSum = 0, ySum = 0;
    uint8_t i;
    for (i = 0; i < 16; i++)
    {
        tSum += readADC(AIN2);
        xSum += readADC(AIN0);
        ySum += readADC(AIN1);
        _delay_cycles(800000);
    }
    throttleCenter = tSum / 16;
    steerXCenter = xSum / 16;
    steerYCenter = ySum / 16;

    putsUart0("Calibration done \r\n");
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
        mapJoystickCalibrated(readADC(AIN2), throttleCenter), DEADBAND);
    pkt.steerX = applyDeadband(
        mapJoystickCalibrated(readADC(AIN0), steerXCenter), DEADBAND);
    pkt.steerY = applyDeadband(
        mapJoystickCalibrated(readADC(AIN1), steerYCenter), DEADBAND);
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
    return ((uint32_t)(pkt->throttle & 0xFF) << 24) |
           ((uint32_t)(pkt->steerX   & 0xFF) << 16) |
           ((uint32_t)(pkt->steerY   & 0xFF) <<  8);
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

        // Debug print over UART
        putsUart0("T:");
        putintUart0((uint32_t)(pkt.throttle + 100)); // shift for unsigned display
        putsUart0(" X:");
        putintUart0((uint32_t)(pkt.steerX + 100));
        putsUart0(" Y:");
        putintUart0((uint32_t)(pkt.steerY + 100));
        putsUart0("\r\n");


        uint32_t txData = packPacket(&pkt);

        // Print what you're actually sending

        NRF24_Transmit(txData);

        _delay_cycles(800000); // ~10ms at 80MHz, simple polling delay
    }
}
