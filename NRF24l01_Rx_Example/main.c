#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "spi0.h"
#include "CommonTerminalInterface.h"
#include "strings.h"
#include "uart0.h"
#include "clock80.h"
#include "NRF.h"


uint8_t RxAddress[] = {0x00,0xDD,0xCC,0xBB,0xAA};
uint32_t RxData;


int main(void)
{

    initSystemClockTo40Mhz();

    initUart0();
    setUart0BaudRate(115200, 40000000);
    initSpi0(USE_SSI0_RX);
    setSpi0BaudRate(8000000, 40000000);
    setSpi0Mode(0, 0);
    NRF24_Init();



    NRF24_RxMode(RxAddress, 10);



    while (1)
    {

        if(isDataAvailable(2) == 1){
            RxData = NRF24_Receive();
            putcUart0(RxData);
            putcUart0(RxData>>8);
        }


    }
    return 0;
}
