#include <stdint.h>
#include "NRF.h"
#include "spi0.h"
#include "tm4c123gh6pm.h"

#define CSN (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 2*4))) //PD2
#define CE (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 3*4))) //PD3

void init_ce_csn()
{
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0 | SYSCTL_RCGCGPIO_R3;
    GPIO_PORTD_DIR_R |= 0x0C;

    GPIO_PORTD_DEN_R |= 0x0C;
}

void CS_Select()
{
    CSN = 0;
}

void CS_UnSelect()
{
    CSN = 1;
}

void CE_Enable()
{
    CE = 1;

}

void CE_Disable()
{
    CE = 0;

}

// write a single byte to the particular register
void nrf24_WriteReg(uint8_t Reg, uint8_t Data)
{
    Reg = Reg | 1 << 5;

    // Pull the CS Pin LOW to select the device
    CS_Select();

    writeSpi0Data(Reg);
    uint8_t data = SSI0_DR_R;
    writeSpi0Data(Data);
    data = SSI0_DR_R;

    // Pull the CS HIGH to release the device
    CS_UnSelect();
}

//write multiple bytes starting from a particular register
void nrf24_WriteRegMulti(uint8_t Reg, uint8_t *data, int size)
{

    int i = 0;
    Reg = Reg | 1 << 5;

    // Pull the CS Pin LOW to select the device
    CS_Select();

    writeSpi0Data(Reg);
    uint8_t temp = SSI0_DR_R;
    for (i = 0; i < size; i++)
    {
        writeSpi0Data(data[i]);
        temp = SSI0_DR_R;
    }

    // Pull the CS HIGH to release the device
    CS_UnSelect();
}

uint8_t nrf24_ReadReg(uint8_t Reg)
{
    uint8_t data = 0;

    CS_Select();

    writeSpi0Data(Reg);
    data = SSI0_DR_R;
    writeSpi0Data(0x00);
    data = SSI0_DR_R;

    CS_UnSelect();

    return data;
}

/* Read multiple bytes from the register /
 void nrf24_ReadReg_Multi (uint8_t Reg, uint8_t *data, int size)
 {
 // Pull the CS Pin LOW to select the device
 CS_Select();

 spi1_transmit(&Reg, 1);
 spi1_receive(data, size);
 // Pull the CS HIGH to release the device
 CS_UnSelect();
 }
 */

// send the command to the NRF
void nrfsendCmd(uint8_t cmd)
{
    // Pull the CS Pin LOW to select the device
    CS_Select();

    writeSpi0Data(cmd);
    uint32_t temp = SSI0_DR_R;

    // Pull the CS HIGH to release the device
    CS_UnSelect();
}

void NRF24_Init(void)
{

    init_ce_csn();
    CE_Disable();

    nrf24_WriteReg(CONFIG, 0);

    nrf24_WriteReg(EN_AA, 0);  // No Auto ACK

    nrf24_WriteReg(EN_RXADDR, 0);  // Not Enabling any data pipe right now

    nrf24_WriteReg(SETUP_AW, 0x03);  // 5 Bytes for the TX/RX address

    nrf24_WriteReg(SETUP_RETR, 0);   // No retransmission

    nrf24_WriteReg(RF_CH, 0);  // will be setup during Tx or RX

    nrf24_WriteReg(RF_SETUP, 0x0E);   // Power= 0db, data rate = 2Mbps

    CE_Enable();

}

void NRF24_TxMode(uint8_t *Address, uint8_t channel)
{
    CE_Disable();
    nrf24_WriteReg(RF_CH, channel);  // select the channel
    nrf24_WriteRegMulti(TX_ADDR, Address, 5);  // Write the TX address
    uint8_t config = nrf24_ReadReg(CONFIG);
    config = config | (1 << 1);   // write 1 in the PWR_UP bit
    nrf24_WriteReg(CONFIG, config);
    CE_Enable();
}

uint8_t NRF24_Transmit(uint32_t Data)
{
    uint8_t cmdtosend = 0;
    uint8_t data = 0;

    CS_Select();
    // payload command
    cmdtosend = W_TX_PAYLOAD;
    writeSpi0Data(cmdtosend);
    data = SSI0_DR_R;

    // send the payload
    writeSpi0Data(Data>>24);
    data = SSI0_DR_R;
    writeSpi0Data(Data>>16);
    data = SSI0_DR_R;
    writeSpi0Data(Data>>8);
    data = SSI0_DR_R;
    writeSpi0Data(Data);
    data = SSI0_DR_R;
    // Unselect the device
    CS_UnSelect();

    uint32_t fifostatus = nrf24_ReadReg(FIFO_STATUS);

    // check the fourth bit of FIFO_STATUS to know if the TX fifo is empty
    if ((fifostatus & (1 << 4)) && (!(fifostatus & (1 << 3))))
    {
        cmdtosend = FLUSH_TX;
        nrfsendCmd(cmdtosend);

        return 1;
    }

    return 0;
}

void NRF24_RxMode(uint8_t *Address, uint8_t channel)
{
    CE_Disable();

    nrf24_WriteReg(RF_CH, channel);  // select the channel

    uint8_t en_rxaddr = nrf24_ReadReg(EN_RXADDR);
    en_rxaddr = en_rxaddr | (1 << 2);
    nrf24_WriteReg(EN_RXADDR, en_rxaddr);

    nrf24_WriteRegMulti(RX_ADDR_P1, Address, 5);  // Write the Pipe1 address

    nrf24_WriteReg(RX_ADDR_P2, 0xEE);  // Write the Pipe2 LSB address

    nrf24_WriteReg(RX_PW_P2, 4);   // 32 bit payload size for pipe 2

    // power up the device in Rx mode
    uint8_t config = nrf24_ReadReg(CONFIG);
    config = config | (1 << 1) | (1 << 0);
    nrf24_WriteReg(CONFIG, config);

    CE_Enable();

}
uint32_t NRF24_Receive()
{

    uint32_t data = 0;
    uint8_t cmdtosend = 0;

    // select the device
    CS_Select();

    // payload command
    cmdtosend = R_RX_PAYLOAD;
    writeSpi0Data(cmdtosend);
    data = SSI0_DR_R;

    // Receive the payload
    writeSpi0Data(0x00);
    data = 0;
    data |= readSpi0Data();
    writeSpi0Data(0x00);
    data |= readSpi0Data() << 8;
    writeSpi0Data(0x00);
    data |= readSpi0Data() << 16;
    writeSpi0Data(0x00);
    data |= readSpi0Data() << 24;

    // Unselect the device
    CS_UnSelect();

    cmdtosend = FLUSH_RX;
    nrfsendCmd(cmdtosend);

    return data;
}

uint8_t isDataAvailable(int pipenum)
{
    uint8_t status = nrf24_ReadReg(STATUS);

    if ((status & (1 << 6)) && (status & (pipenum << 1)))
    {

        nrf24_WriteReg(STATUS, (1 << 6));

        return 1;
    }

    return 0;

}

