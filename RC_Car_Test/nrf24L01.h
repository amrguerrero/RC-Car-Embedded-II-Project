#ifndef NRF24L01_H_
#define NRF24L01_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "spi1.h"
#include "gpio.h"

// -----------------------------
// Pin Definitions
// -----------------------------
#define NRF_IRQ PORTB, 3
#define NRF_CE  PORTB, 4
#define NRF_CSN PORTD, 1   

/// -----------------------------
// nRF24L01 commands
// -----------------------------
#define NRF_CMD_R_REGISTER        0x00
#define NRF_CMD_W_REGISTER        0x20
#define NRF_CMD_R_RX_PAYLOAD      0x61
#define NRF_CMD_W_TX_PAYLOAD      0xA0
#define NRF_CMD_FLUSH_TX          0xE1
#define NRF_CMD_FLUSH_RX          0xE2
#define NRF_CMD_NOP               0xFF

// -----------------------------
// Registers
// -----------------------------
#define NRF_REG_CONFIG        0x00
#define NRF_REG_EN_AA         0x01
#define NRF_REG_EN_RXADDR     0x02
#define NRF_REG_SETUP_AW      0x03
#define NRF_REG_SETUP_RETR    0x04
#define NRF_REG_RF_CH         0x05
#define NRF_REG_RF_SETUP      0x06
#define NRF_REG_STATUS        0x07
#define NRF_REG_RX_ADDR_P0    0x0A
#define NRF_REG_TX_ADDR       0x10
#define NRF_REG_RX_PW_P0      0x11
#define NRF_REG_FIFO_STATUS   0x17
#define NRF_REG_DYNPD         0x1C
#define NRF_REG_FEATURE       0x1D

// STATUS bits
#define NRF_STATUS_RX_DR  (1 << 6)
#define NRF_STATUS_TX_DS  (1 << 5)
#define NRF_STATUS_MAX_RT (1 << 4)

// CONFIG bits
#define NRF_CONFIG_EN_CRC  (1 << 3)
#define NRF_CONFIG_PWR_UP  (1 << 1)
#define NRF_CONFIG_PRIM_RX (1 << 0)

#define MAX_PAYLOAD_SIZE 32
#define NRF_TIMEOUT 100000

//-----------------------------------------------------------------------------
// Typedef
//-----------------------------------------------------------------------------
typedef enum
{
    STOP,
    FORWARD,
    REVERSE,
    ANGLE
} nrfCommand_t;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

#endif
