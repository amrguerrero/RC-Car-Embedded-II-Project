#include "nrf24L01.h"

// -----------------------------
// SPI Transfer
// -----------------------------
uint8_t spiTransfer(uint8_t data)
{
    writeSpi1Data(data);
    return (uint8_t)readSpi1Data();
}

// -----------------------------
// Register Access
// -----------------------------
uint8_t nrfReadReg(uint8_t reg)
{
    uint8_t value;

    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_R_REGISTER | (reg & 0x1F));
    value = spiTransfer(NRF_CMD_NOP);
    setPinValue(NRF_CSN, 1);

    return value;
}

void nrfWriteReg(uint8_t reg, uint8_t value)
{
    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_W_REGISTER | (reg & 0x1F));
    spiTransfer(value);
    setPinValue(NRF_CSN, 1);
}

void nrfWriteBuf(uint8_t reg, uint8_t *data, uint8_t len)
{
    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_W_REGISTER | (reg & 0x1F));

    uint8_t i;
    for (i= 0; i < len; i++)
        spiTransfer(data[i]);

    setPinValue(NRF_CSN, 1);
}

// -----------------------------
// FIFO Control
// -----------------------------
void nrfFlushTx()
{
    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_FLUSH_TX);
    setPinValue(NRF_CSN, 1);
}

void nrfFlushRx()
{
    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_FLUSH_RX);
    setPinValue(NRF_CSN, 1);
}

// -----------------------------
// Init
// -----------------------------
void nrfInit(bool isRx)
{
    setPinValue(NRF_CE, 0);
    setPinValue(NRF_CSN, 1);
    waitMicrosecond(5000);

    // Disable auto-ack (simple mode)
    nrfWriteReg(NRF_REG_EN_AA, 0x00);

    // Enable pipe 0
    nrfWriteReg(NRF_REG_EN_RXADDR, 0x01);

    // Address width = 5 bytes
    nrfWriteReg(NRF_REG_SETUP_AW, 0x03);

    // No retransmit
    nrfWriteReg(NRF_REG_SETUP_RETR, 0x00);

    // Channel
    nrfWriteReg(NRF_REG_RF_CH, 76);

    // 1Mbps, 0dBm
    nrfWriteReg(NRF_REG_RF_SETUP, 0x07);

    // Address
    uint8_t addr[5] = {'N','O','D','E','1'};
    nrfWriteBuf(NRF_REG_TX_ADDR, addr, 5);
    nrfWriteBuf(NRF_REG_RX_ADDR_P0, addr, 5);

    // Payload size = 32 bytes
    nrfWriteReg(NRF_REG_RX_PW_P0, MAX_PAYLOAD_SIZE);

    // Disable dynamic payloads
    nrfWriteReg(NRF_REG_FEATURE, 0);
    nrfWriteReg(NRF_REG_DYNPD, 0);

    // Clear interrupts
    nrfWriteReg(NRF_REG_STATUS, 0x70);

    // Power up
    if (isRx)
    {
        nrfWriteReg(NRF_REG_CONFIG,
            NRF_CONFIG_PWR_UP |
            NRF_CONFIG_PRIM_RX |
            NRF_CONFIG_EN_CRC);
    }
    else
    {
        nrfWriteReg(NRF_REG_CONFIG,
            NRF_CONFIG_PWR_UP |
            NRF_CONFIG_EN_CRC);
    }

    waitMicrosecond(1500);

    if (isRx)
        setPinValue(NRF_CE, 1);
}

// -----------------------------
// Send (32-byte payload)
// -----------------------------
bool nrfSend(uint8_t *data)
{
    uint8_t status;
    int timeout = NRF_TIMEOUT;

    setPinValue(NRF_CE, 0);
    nrfFlushTx();

    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_W_TX_PAYLOAD);

    int i;
    for (i = 0; i < MAX_PAYLOAD_SIZE; i++)
        spiTransfer(data[i]);

    setPinValue(NRF_CSN, 1);

    // Start TX
    setPinValue(NRF_CE, 1);
    waitMicrosecond(20);
    setPinValue(NRF_CE, 0);

    // Wait for IRQ with timeout
    while (getPinValue(NRF_IRQ) && timeout--);

    if (timeout <= 0)
        return false;

    status = nrfReadReg(NRF_REG_STATUS);

    // Clear interrupts
    nrfWriteReg(NRF_REG_STATUS, 0x70);

    if (status & NRF_STATUS_MAX_RT)
    {
        nrfFlushTx();
        return false;
    }

    return (status & NRF_STATUS_TX_DS);
}

// -----------------------------
// Check if data available
// -----------------------------
bool nrfAvailable()
{
    return !getPinValue(NRF_IRQ);
}

// -----------------------------
// Receive (32-byte payload)
// -----------------------------
bool nrfReceive(uint8_t *buffer)
{
    if (!(nrfReadReg(NRF_REG_STATUS) & NRF_STATUS_RX_DR))
        return false;

    setPinValue(NRF_CSN, 0);
    spiTransfer(NRF_CMD_R_RX_PAYLOAD);

    int i;
    for (i = 0; i < MAX_PAYLOAD_SIZE; i++)
        buffer[i] = spiTransfer(NRF_CMD_NOP);

    setPinValue(NRF_CSN, 1);

    // Clear RX interrupt
    nrfWriteReg(NRF_REG_STATUS, NRF_STATUS_RX_DR);

    return true;
}
