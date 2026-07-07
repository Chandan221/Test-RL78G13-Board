/******************************************************************************
 * i2c_hal.c  --  I2C Bit-Bang Hardware Abstraction Layer
 *
 * Implements a standard-mode (100 kHz) I2C master using GPIO bit-banging
 * on the IICA0 dedicated pins (P60 = SCLA0, P61 = SDAA0).  The RL78/G13
 * SFR registers are accessed via their raw memory-mapped addresses (see
 * SFR_MAP comment below) so that no Code Generator struct definitions
 * are required.
 *
 * Protocol reference:
 *   - Start condition:    SDA falls while SCL is high
 *   - Data bit:           SDA stable while SCL is high; sampled on rising edge
 *   - ACK:                Master releases SDA; slave pulls low
 *   - Stop condition:     SDA rises while SCL is high
 *
 * Pin assignment  (IICA0 alternate-function pins):
 *   SCL  ->  P60/SCLA0   pin 37  (Port 6, bit 0)
 *   SDA  ->  P61/SDAA0   pin 38  (Port 6, bit 1)
 *
 * These pins are available on the CN1 expansion header.
 * Consult the board manual for the exact CN1 pin numbers.
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SLAFB)
 * Toolchain:  CC-RL  (e2 studio)
 *
 * Revision History:
 *   2026-07-07  Initial version
 ******************************************************************************/
#include "i2c_hal.h"

/******************************************************************************
 * SFR Memory Map  (R5F100SLAFB -- RL78/G13, 128-pin)
 *
 *  Address    Symbol   Description
 *  ---------  -------  --------------------------------------------
 *  0xFFF06    P6       Port 6  data register      (read/write)
 *  0xFFF26    PM6      Port 6  mode register       (0=output, 1=input)
 *
 * Port 6 pin functions on QB-R5F100SL-TB:
 *   P60  (bit 0)  ->  SCLA0  (IICA0 clock)   -- pin 37
 *   P61  (bit 1)  ->  SDAA0  (IICA0 data)    -- pin 38
 *   P62  (bit 2)  ->  SCLA1  (IICA1 clock)
 *   P63  (bit 3)  ->  SDAA1  (IICA1 data)
 *   P64--P67      ->  Timer I/O 10--13
 ******************************************************************************/
#pragma warning(disable:W0520171)
static volatile unsigned char * const p6   = (unsigned char *)0xFFF06;
static volatile unsigned char * const pm6  = (unsigned char *)0xFFF26;
#pragma warning(default:W0520171)
#define P6_REG   (*p6)
#define PM6_REG  (*pm6)

#define SCL_PIN   0x01U   /* P60/SCLA0 -- bit 0 */
#define SDA_PIN   0x02U   /* P61/SDAA0 -- bit 1 */

/* Clock-stretch timeout in busy-wait iterations (~100 ms at 20 MHz) */
#define I2C_TIMEOUT  10000U

/******************************************************************************
 * Private:  Bus-level primitives
 ******************************************************************************/

/* ---- SDA / SCL primitives ---------------------------------------------- */

/**
 * Drive SCL low or high.
 *
 * @param[in] level  0 = low (sink), 1 = high (released with pull-up).
 */
static void i2c_scl(uint8_t level)
{
    if (level)
        P6_REG |=  SCL_PIN;
    else
        P6_REG &= ~SCL_PIN;
}

/**
 * Drive SDA low or high.
 *
 * @param[in] level  0 = low (sink), 1 = high (released with pull-up).
 */
static void i2c_sda(uint8_t level)
{
    if (level)
        P6_REG |=  SDA_PIN;
    else
        P6_REG &= ~SDA_PIN;
}

/**
 * Read the current SDA pin level.
 *
 * @return  1 if SDA is high, 0 if low.
 */
static uint8_t i2c_read_sda(void)
{
    return (uint8_t)((P6_REG & SDA_PIN) != 0U);
}

/**
 * Release SCL (driven high) and wait for clock stretch by the slave.
 *
 * If the slave holds SCL low (clock stretching), this function polls
 * until SCL goes high or a timeout expires.
 *
 * @return  1 if SCL rose (OK), 0 if timeout (slave stuck).
 */
static uint8_t i2c_release_scl(void)
{
    uint16_t timeout = I2C_TIMEOUT;

    P6_REG |= SCL_PIN;               /* drive SCL high */
    while ((P6_REG & SCL_PIN) == 0U) /* wait for release */
    {
        if (--timeout == 0U)
            return 0U;               /* timeout */
    }
    return 1U;                       /* OK */
}

/* ---- Bus-level operations ---------------------------------------------- */

/**
 * Generate an I2C START condition:
 *   SDA goes low while SCL is high.
 */
static void i2c_start(void)
{
    i2c_sda(1U);                     /* SDA high */
    i2c_scl(1U);                     /* SCL high */
    i2c_sda(0U);                     /* SDA falling while SCL high */
    i2c_scl(0U);                     /* SCL low for data */
}

/**
 * Generate an I2C STOP condition:
 *   SDA goes high while SCL is high.
 */
static void i2c_stop(void)
{
    i2c_scl(0U);                     /* ensure SCL low first */
    i2c_sda(0U);                     /* SDA low */
    i2c_scl(1U);                     /* SCL rising */
    i2c_sda(1U);                     /* SDA rising while SCL high */
}

/**
 * Transmit one byte on the I2C bus and read the ACK bit.
 *
 * @param[in] data  Byte to send.
 * @return  1 if slave ACK'd (SDA low), 0 if NACK.
 */
static uint8_t i2c_tx_byte(uint8_t data)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; i++)
    {
        /* Set SDA while SCL is low */
        if (data & 0x80U)
            i2c_sda(1U);
        else
            i2c_sda(0U);

        if (i2c_release_scl() == 0U)
            return 0U;               /* clock stretch timeout */

        i2c_scl(0U);                 /* SCL low for next bit */
        data <<= 1U;
    }

    /* Release SDA for slave ACK */
    i2c_sda(1U);
    if (i2c_release_scl() == 0U)
        return 0U;                   /* clock stretch timeout */

    ack = (uint8_t)((P6_REG & SDA_PIN) == 0U) ? 1U : 0U;
    i2c_scl(0U);                     /* SCL low before next operation */
    return ack;
}

/**
 * Read one byte from the I2C bus and send ACK or NACK.
 *
 * @param[in] send_ack  1 = send ACK (expect more data),
 *                      0 = send NACK (last byte).
 * @return  The received byte.
 */
static uint8_t i2c_rx_byte(uint8_t send_ack)
{
    uint8_t i;
    uint8_t data = 0U;

    i2c_sda(1U);                     /* release SDA for slave to drive */

    for (i = 0U; i < 8U; i++)
    {
        data <<= 1U;
        if (i2c_release_scl() == 0U)
            return 0U;               /* clock stretch timeout */

        if ((P6_REG & SDA_PIN) != 0U)
            data |= 0x01U;

        i2c_scl(0U);                 /* SCL low for next bit */
    }

    /* ACK / NACK */
    if (send_ack)
        i2c_sda(0U);                 /* ACK = SDA low */
    else
        i2c_sda(1U);                 /* NACK = SDA high */

    if (i2c_release_scl() == 0U)
        return 0U;

    i2c_scl(0U);                     /* SCL low */
    i2c_sda(1U);                     /* release SDA */
    return data;
}

/******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * Initialise the I2C bus.
 *
 * Configures P60 (SCL) and P61 (SDA) as open-drain outputs.
 * The bus is left idle (SCL high, SDA high).
 */
void I2C_HAL_Init(void)
{
    /* Set P60 and P61 as outputs (PM6 bit 0, bit 1 cleared) */
    PM6_REG &= ~(SCL_PIN | SDA_PIN);    /* both pins output */
    P6_REG  |=   SCL_PIN | SDA_PIN;     /* bus idle high */
}

/**
 * Read a sequence of bytes from an I2C slave device.
 *
 * Generates: START + slave_address(W) + register_address + STOP +
 *            START + slave_address(R) + data_bytes + STOP.
 *
 * @param[in]  dev_addr   7-bit slave address (left-justified).
 * @param[in]  reg_addr   Internal register address to read from.
 * @param[out] data       Output buffer for received bytes.
 * @param[in]  len        Number of bytes to read.
 * @return  0 on success, 1 on NACK or bus error.
 */
uint8_t I2C_HAL_Read(uint8_t dev_addr, uint8_t reg_addr,
                      uint8_t *data, uint16_t len)
{
    uint16_t i;

    i2c_start();

    /* Write-phase: address + register */
    if (i2c_tx_byte(dev_addr & 0xFEU) == 0U)   /* write, LSB = 0 */
        return 1U;

    if (i2c_tx_byte(reg_addr) == 0U)
        return 1U;

    /* Repeated START */
    i2c_start();

    /* Read-phase: address + data */
    if (i2c_tx_byte(dev_addr | 0x01U) == 0U)   /* read, LSB = 1 */
        return 1U;

    for (i = 0U; i < len; i++)
    {
        uint8_t ack = (i < (len - 1U)) ? 1U : 0U;
        data[i] = i2c_rx_byte(ack);
    }

    i2c_stop();
    return 0U;
}

/**
 * Write a sequence of bytes to an I2C slave device.
 *
 * Generates: START + slave_address(W) + register_address + data_bytes + STOP.
 *
 * @param[in]  dev_addr   7-bit slave address (left-justified).
 * @param[in]  reg_addr   Internal register address to write to.
 * @param[in]  data       Data buffer to transmit.
 * @param[in]  len        Number of bytes to write.
 * @return  0 on success, 1 on NACK or bus error.
 */
uint8_t I2C_HAL_Write(uint8_t dev_addr, uint8_t reg_addr,
                       const uint8_t *data, uint16_t len)
{
    uint16_t i;

    i2c_start();

    if (i2c_tx_byte(dev_addr & 0xFEU) == 0U)   /* write, LSB = 0 */
        return 1U;

    if (i2c_tx_byte(reg_addr) == 0U)
        return 1U;

    for (i = 0U; i < len; i++)
    {
        if (i2c_tx_byte(data[i]) == 0U)
            return 1U;
    }

    i2c_stop();
    return 0U;
}

/**
 * Write a single byte to an I2C slave register (convenience wrapper).
 *
 * @param[in] dev_addr  7-bit slave address (left-justified).
 * @param[in] reg_addr  Internal register address.
 * @param[in] val       Byte to write.
 * @return  0 on success, 1 on error.
 */
uint8_t I2C_HAL_WriteByte(uint8_t dev_addr, uint8_t reg_addr, uint8_t val)
{
    return I2C_HAL_Write(dev_addr, reg_addr, &val, 1U);
}

/**
 * Write then read a sequence of bytes from an I2C slave (combined format).
 *
 * Generates: START + addr(W) + reg_addr + (optional repeated start) +
 *            START + addr(R) + data_bytes + STOP.
 *
 * When reg_addr_len > 0, the register address is sent first in the
 * write phase.  When reg_addr_len == 0, the write phase is omitted
 * and only the read phase occurs (useful for devices that auto-increment).
 *
 * @param[in]  dev_addr     7-bit slave address (left-justified).
 * @param[in]  reg_addr     Pointer to register address bytes to send.
 * @param[in]  reg_addr_len Number of register address bytes (0 to skip).
 * @param[out] rx_data      Output buffer for received bytes.
 * @param[in]  rx_len       Number of bytes to read.
 * @return  0 on success, 1 on NACK or bus error.
 */
uint8_t I2C_HAL_WriteRead(uint8_t dev_addr,
                           const uint8_t *reg_addr, uint16_t reg_addr_len,
                           uint8_t *rx_data, uint16_t rx_len)
{
    uint16_t i;

    i2c_start();

    if (i2c_tx_byte(dev_addr & 0xFEU) == 0U)   /* write, LSB = 0 */
        return 1U;

    for (i = 0U; i < reg_addr_len; i++)
    {
        if (i2c_tx_byte(reg_addr[i]) == 0U)
            return 1U;
    }

    /* Repeated START */
    i2c_start();

    if (i2c_tx_byte(dev_addr | 0x01U) == 0U)   /* read, LSB = 1 */
        return 1U;

    for (i = 0U; i < rx_len; i++)
    {
        uint8_t ack = (i < (rx_len - 1U)) ? 1U : 0U;
        rx_data[i] = i2c_rx_byte(ack);
    }

    i2c_stop();
    return 0U;
}
