#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "spi0.h"
#include "CommonTerminalInterface.h"
#include "strings.h"
#include "uart0.h"
#include "clock80.h"
#include "NRF.h"

#define IRQ (*((volatile uint32_t *)(0x42000000 + (0x4000.53FC-0x40000000)*32 + 3*4))) //PB3

#define IRQ_MASK 0x00000008

uint8_t RxAddress[] = { 0x00, 0xDD, 0xCC, 0xBB, 0xAA };
uint8_t TxAddress[] = { 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };

uint32_t RxData;
uint32_t TxData = 0x12345678;

void NRFIRQ(void)
{
    if (isDataAvailable(2))
    {
        RxData = NRF24_Receive();
        putcUart0(RxData);
        putcUart0(RxData >> 8);
        putcUart0(RxData >> 16);
        putcUart0(RxData >> 24);
    }

    GPIO_PORTB_ICR_R = IRQ_MASK;

}

int main(void)
{

    initSystemClockTo40Mhz();

    initUart0();
    setUart0BaudRate(115200, 40000000);
    initSpi0(USE_SSI0_RX);
    setSpi0BaudRate(8000000, 40000000);
    setSpi0Mode(0, 0);
    NRF24_Init();

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;

    GPIO_PORTB_DEN_R |= IRQ_MASK;
    GPIO_PORTB_DIR_R &= ~(IRQ_MASK);

    GPIO_PORTB_IS_R &= ~IRQ_MASK;    // edge triggered
    GPIO_PORTB_IBE_R |= IRQ_MASK;   // not both edges
    GPIO_PORTB_IEV_R &= ~IRQ_MASK;    // falling edge
    GPIO_PORTB_ICR_R |= IRQ_MASK;   // clear it
    GPIO_PORTB_IM_R |= IRQ_MASK;    // arm interrupt

    //NVIC_EN0_R |= 0x00000002;

    //NRF24_RxMode(RxAddress, 10);
    NRF24_TxMode(TxAddress, 10);

    while (1)
    {
        NRF24_Transmit(TxData);
        /*
        if (isDataAvailable(2))
        {
            RxData = NRF24_Receive();
            putcUart0(RxData);
            putcUart0(RxData >> 8);
            putcUart0(RxData >> 16);
            putcUart0(RxData >> 24);
        }
        */
    }
    return 0;
}
