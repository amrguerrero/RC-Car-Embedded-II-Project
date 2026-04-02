/*
 * H Bridge Pinout:
 * PB0 - Output for 1,2 EN
 * PF2 - PWM for A1
 * PF3 - PWM for A2
 *
 * Ultrasonic Sensor Pinout:
 * PB1 - Output for Trig for
 * PB2 - Input for Echo
 *
 * Servo Pinout:
 * PF1 - PWM for Servo
 *
 * NRF24L01 Pinout:
 * PB3 - Interrupt
 * PB4 - CE
 * PD0 - SCK
 * PD1 - CSN
 * PD2 - MISO
 * PD3 - MOSI
 */
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "spi1.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "clock80.h"
#include "CTI.h"

// defining pin masks
#define HBRIDGE_EN PORTB, 0
#define TRIG PORTB, 1
#define ECHO PORTB, 2
#define NRF_IRQ PORTB, 3
#define NRF_CE  PORTB, 4
#define NRF_CSN PORTD, 1

// PF2 and PF3 for H-Bridge PWM, since they share a load value
// PF1 for Servo PWM, since it can have a different load value
void initPWM(void)
{
	// Enable clocks
	SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
	_delay_cycles(3);

	// PF1 → M1PWM5
	// PF2 → M1PWM6
	// PF3 → M1PWM7
	setPinAuxFunction(PORTF, 1, 5); // PCTL = 0x5
	setPinAuxFunction(PORTF, 2, 5);
	setPinAuxFunction(PORTF, 3, 5);

	selectPinPushPullOutput(PORTF, 1);
	selectPinPushPullOutput(PORTF, 2);
	selectPinPushPullOutput(PORTF, 3);

	SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1; // reset PWM1 module
	SYSCTL_SRPWM_R = 0;				  // leave reset state
	PWM1_2_CTL_R = 0;				  // turn-off PWM1 generator 2 (drives outs 4 and 5)
	PWM1_3_CTL_R = 0;				  // turn-off PWM1 generator 3 (drives outs 6 and 7)
	PWM1_2_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
	// output 5 on PWM1, gen 2b, cmpb
	PWM1_3_GENA_R = PWM_1_GENA_ACTCMPAD_ONE | PWM_1_GENA_ACTLOAD_ZERO;
	// output 6 on PWM1, gen 3a, cmpa
	PWM1_3_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
	// output 7 on PWM1, gen 3b, cmpb

	// -------------------------
	// SERVO PWM (PF1)
	// -------------------------
	PWM1_2_LOAD_R = 400000 - 1;		// 20 ms period (50 Hz)
	PWM1_2_CMPB_R = 400000 - 30000; // default 1500 us center

	// -------------------------
	// H-BRIDGE PWM (PF2, PF3)
	// -------------------------
	PWM1_3_LOAD_R = 1024; // ~20 kHz
	PWM1_3_CMPA_R = 0;
	PWM1_3_CMPB_R = 0;

	// Enable generators
	PWM1_2_CTL_R |= PWM_1_CTL_ENABLE;
	PWM1_3_CTL_R |= PWM_1_CTL_ENABLE;

	// Enable outputs
	PWM1_ENABLE_R = PWM_ENABLE_PWM5EN | PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN;
}

void setServo(uint16_t angle)
{
	// Map angle (0-180) to pulse width (500-2500 us)
	if (angle > 180)
		angle = 180;
	uint16_t pulseWidth = 500 + ((uint32_t)angle * 2000) / 180;
	PWM1_2_CMPB_R = 400000 - pulseWidth;
}

void setHBridge(uint8_t direction, uint8_t speed)
{
	// direction: 0 = stop, 1 = forward, 2 = reverse
	// speed: 0-100%
	if (direction == 0)
	{
		PWM1_3_CMPA_R = 0;
		PWM1_3_CMPB_R = 0;
	}
	else if (direction == 1)
	{
		PWM1_3_CMPA_R = 0;
		PWM1_3_CMPB_R = (speed * PWM1_3_LOAD_R) / 100;
	}
	else if (direction == 2)
	{
		PWM1_3_CMPA_R = (speed * PWM1_3_LOAD_R) / 100;
		PWM1_3_CMPB_R = 0;
	}
}

void initHardware(void)
{
	// Initialize system clock 40 Mhz
	initSystemClockTo40Mhz();

	// enable Peripheral Clocks
	enablePort(PORTB);
	enablePort(PORTD);
	enablePort(PORTF);

	// Set up normal GPIO pins
	selectPinPushPullOutput(HBRIDGE_EN);
	selectPinPushPullOutput(NRF_CE);
	selectPinPushPullOutput(TRIG);
	selectPinDigitalInput(ECHO);
	selectPinDigitalInput(NRF_IRQ);

	// Set up SPI1 for NRF24L01
	initSpi1(USE_SSI_RX);
	setSpi1BaudRate(1000000, 40000000); // 1 MHz
	setSpi1Mode(0, 0);					// Mode 0 (NRF requires this)

	// Set up UART0 for debugging
	initUart0();
	setUart0BaudRate(115200, 40000000);

	// Init PWM for H-Bridge and Servo
	initPWM();
}

void printCommandTable(void)
{
    putsUart0("\r\nCommand Line Interface - RC CAR\r\n");
    putsUart0("---------------------------------------------------------------------------------------\r\n");
    putsUart0("CMD    | PARAMETERS                     | PURPOSE\r\n");
    putsUart0("---------------------------------------------------------------------------------------\r\n");
    putsUart0("DC     | Direction & Duty Cycle (0-100) | Set DC Motor\r\n");
    putsUart0("SERVO  | Angle (0-180)                  | Set Angle\r\n");
    putsUart0("HELP   |                                | Print Command Table\r\n");
    putsUart0("---------------------------------------------------------------------------------------\r\n");
}

void handleDC(USER_DATA *data)
{
	if (data->fieldCount < 2)
	{
		putsUart0("Usage: DC <Direction> <Duty Cycle>\r\n");
		return;
	}

	char *directionStr = getFieldString(data, 1);
	uint8_t dutyCycle = getFieldInteger(data, 2);

	uint8_t direction;
	if (strcmp(directionStr, "STOP") == 0)
		direction = 0;
	else if (strcmp(directionStr, "FORWARD") == 0)
		direction = 1;
	else if (strcmp(directionStr, "REVERSE") == 0)
		direction = 2;
	else
	{
		putsUart0("Invalid direction. Use STOP, FORWARD, or REVERSE.\r\n");
		return;
	}

	if (dutyCycle > 100)
	{
		putsUart0("Invalid duty cycle. Must be between 0 and 100.\r\n");
		return;
	}

	setHBridge(direction, dutyCycle);
}

void handleServo(USER_DATA *data)
{
	if (data->fieldCount < 1)
	{
		putsUart0("Usage: SERVO <Angle>\r\n");
		return;
	}

	uint16_t angle = getFieldInteger(data, 1);

	if (angle > 180)
	{
		putsUart0("Invalid angle. Must be between 0 and 180.\r\n");
		return;
	}

	setServo(angle);
}

void processCommand(USER_DATA *data)
{
	if (data->fieldCount == 0)
		return;

	if (isCommand(data, "DC", 2))
		handleDC(data);
	else if (isCommand(data, "SERVO", 1))
		handleServo(data);
	else if (isCommand(data, "HELP", 0))
        printCommandTable();
	else
		putsUart0("Invalid command. Type HELP.\r\n");
}

bool test = false;
int main(void)
{
    initHardware();
    if (test)
    {
        // Using a CLI to send and receive commands over UART
        USER_DATA data;
        printCommandTable();

            while (1)
            {
                putsUart0("\r\n> ");
                getsUart0(&data);

                putsUart0(data.buffer);
                putsUart0("\n\r'");

                parseFields(&data);

        #ifdef DEBUG
                uint8_t i;
                for (i = 0; i < data.fieldCount; i++)
                {
                    putcUart0(data.fieldType[i]);
                    putcUart0('\t');
                    putsUart0(&data.buffer[data.fieldPosition[i]]);
                    putsUart0("\n\r'");
                }
        #endif

                processCommand(&data);
            }
    }
	return 0;
}
