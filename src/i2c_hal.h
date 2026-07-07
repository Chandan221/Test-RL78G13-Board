/******************************************************************************
 * i2c_hal.h  --  I2C Bit-Bang Hardware Abstraction Layer
 *
 * Provides master-mode I2C read/write operations using GPIO bit-banging on
 * the RL78/G13.  Uses the IICA0-dedicated pins (P60/P61) so the physical
 * connection matches the on-chip I2C peripheral, but no hardware I2C
 * peripheral registers are touched -- this is pure GPIO bit-bang.
 *
 * Pin assignment  (IICA0 alternate-function pins):
 *   SCL  ->  P60/SCLA0   (pin 37 -- bottom edge, CN1)
 *   SDA  ->  P61/SDAA0   (pin 38 -- bottom edge, CN1)
 *
 * Timing targets ~100 kHz standard-mode I2C.
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SLAFB)
 * Toolchain:  CC-RL  (e2 studio)
 ******************************************************************************/
#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>

/******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * Initialise I2C GPIO pins.
 *
 * Sets P40 (SDA) and P41 (SCL) to output, idle-high, then issues a bus-stop
 * condition to reset any connected slaves.
 */
void I2C_HAL_Init(void);

/**
 * Write one byte to a device register.
 *
 * Combines a START condition, device-address + W, register-address, data byte,
 * and STOP condition into a single transaction.
 *
 * @param[in] dev_addr  7-bit I2C device address (NOT left-shifted).
 * @param[in] reg_addr  Target register address on the device.
 * @param[in] data      Byte to write.
 * @return 0 on success, nonzero if any NACK was received.
 */
uint8_t I2C_HAL_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

/**
 * Read a sequence of bytes from a device register.
 *
 * Performs a combined write-then-read transaction:
 *   START + dev_addr+W + reg_addr + STOP
 *   START + dev_addr+R + [data bytes] + STOP
 *
 * The last byte is NACKed to signal end-of-read.
 *
 * @param[in]  dev_addr  7-bit I2C device address.
 * @param[in]  reg_addr  First register address to read from.
 * @param[out] buffer    Destination buffer for the received bytes.
 * @param[in]  length    Number of bytes to read (buffer must be >= length).
 * @return 0 on success, nonzero on NACK.
 */
uint8_t I2C_HAL_Read(uint8_t dev_addr, uint8_t reg_addr,
                     uint8_t *buffer, uint8_t length);

#endif /* I2C_HAL_H */
