

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


void initGPIO(void)
{
    SYSCTL_RCGCGPIO_R |= 0x3F; // Enable Port A-F
    _delay_cycles(3);

    // Motor Pins (HOLD)
    GPIO_PORTB_DIR_R |= 0xFF;
    GPIO_PORTB_DEN_R |= 0xFF;

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

}

int main(void)
{
    USER_DATA data;
    initHardware();

    while(1)
    {
        //TESTTING UART-CONTROLLED MOTOR THROTTLE
        putsUart0("Enter duty (0-100): ");
        getsUart0(&data);
        uint8_t duty = atoi(data.buffer);

        if (duty <= 100)
        {
            setPWM0Duty(duty);
            putsUart0("Duty set\r\n");
        }
        else
        {
            putsUart0("Invalid\r\n");
        }
    }
}
