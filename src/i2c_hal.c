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

/* Clock-stretch timeout in busy-wait iterations (~5 ms at 8 MHz) */
#define I2C_TIMEOUT  1000U

/******************************************************************************
 * Private:  Bus-level primitives
 ******************************************************************************/

/**
 * Microsecond-scale delay for I2C timing.
 * Calibrated for 8 MHz HOCO; yields approximately 5 us per call.
 */
static void i2c_delay(void)
{
    volatile uint16_t i;
    for (i = 0; i < 15U; i++)
    {
        /* empty */
    }
}

/**
 * Drive SCL high (output mode, set bit).
 */
static void scl_high(void)
{
    P6_REG |= SCL_PIN;
}

/**
 * Drive SCL low (output mode, clear bit).
 */
static void scl_low(void)
{
    P6_REG &= ~SCL_PIN;
}

/**
 * Release SDA (switch to input -- external pull-up drives it high).
 */
static void sda_release(void)
{
    PM6_REG |= SDA_PIN;         /* input = high-Z */
}

/**
 * Drive SDA low (switch to output, clear data bit).
 */
static void sda_assert(void)
{
    PM6_REG &= ~SDA_PIN;        /* output */
    P6_REG  &= ~SDA_PIN;        /* low */
}

/**
 * Read the current SDA line level.
 *
 * @return 1 if SDA is high, 0 if low.
 */
static uint8_t sda_read(void)
{
    sda_release();
    i2c_delay();
    return (P6_REG & SDA_PIN) ? 1U : 0U;
}

/******************************************************************************
 * Private:  I2C protocol primitives
 ******************************************************************************/

/**
 * Generate START condition: SDA falls while SCL is high.
 */
static void i2c_start(void)
{
    sda_release();
    scl_high();
    i2c_delay();
    sda_assert();
    i2c_delay();
    scl_low();
    i2c_delay();
}

/**
 * Generate STOP condition: SDA rises while SCL is high.
 */
static void i2c_stop(void)
{
    sda_assert();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_release();
    i2c_delay();
}

/**
 * Generate an ACK (master pulls SDA low for one SCL cycle).
 */
static void i2c_ack(void)
{
    scl_low();
    i2c_delay();
    sda_assert();
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();
}

/**
 * Generate a NACK (master leaves SDA high for one SCL cycle).
 */
static void i2c_nack(void)
{
    scl_low();
    i2c_delay();
    sda_release();
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();
}

/**
 * Wait for an ACK from the slave.
 *
 * After transmitting 8 bits, the master releases SDA and checks
 * whether the slave pulls it low (ACK) or leaves it high (NACK).
 *
 * @return 0 if ACK received, 1 if NACK or timeout.
 */
static uint8_t i2c_wait_ack(void)
{
    uint16_t timeout = 0U;

    sda_release();
    i2c_delay();
    scl_high();
    i2c_delay();

    while (sda_read() != 0U)
    {
        timeout++;
        if (timeout > I2C_TIMEOUT)
        {
            scl_low();
            return 1U;                      /* timeout / NACK */
        }
    }

    scl_low();
    i2c_delay();
    return 0U;                              /* ACK received */
}

/******************************************************************************
 * Private:  Byte-level I/O
 ******************************************************************************/

/**
 * Transmit one byte MSB-first over I2C.
 *
 * @param[in] data  Byte to transmit.
 * @return 0 if slave ACKed, 1 if NACKed.
 */
static uint8_t i2c_write_byte(uint8_t data)
{
    int8_t i;

    for (i = 7; i >= 0; i--)
    {
        scl_low();
        i2c_delay();

        if (data & (1U << i))
            sda_release();
        else
            sda_assert();

        i2c_delay();
        scl_high();
        i2c_delay();
    }

    scl_low();
    i2c_delay();
    return i2c_wait_ack();
}

/**
 * Receive one byte MSB-first from I2C.
 *
 * @param[in] send_ack  1 to send ACK (more bytes to follow),
 *                      0 to send NACK (last byte).
 * @return The received byte.
 */
static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t data = 0U;
    int8_t  i;

    sda_release();

    for (i = 7; i >= 0; i--)
    {
        scl_high();
        i2c_delay();

        if (sda_read())
            data |= (1U << i);

        scl_low();
        i2c_delay();
    }

    if (send_ack)
        i2c_ack();
    else
        i2c_nack();

    return data;
}

/******************************************************************************
 * Public API
 ******************************************************************************/

void I2C_HAL_Init(void)
{
    /* P60 (SCL) and P61 (SDA) as outputs, idle high */
    PM6_REG  &= ~(SCL_PIN | SDA_PIN);   /* both output */
    P6_REG   |=  SCL_PIN;              /* SCL high    */
    P6_REG   |=  SDA_PIN;              /* SDA high    */

    i2c_stop();                         /* reset any slaves */
}

uint8_t I2C_HAL_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t status;

    i2c_start();
    status = i2c_write_byte((uint8_t)(dev_addr << 1) | 0x00U);  /* W */
    if (status != 0U) { i2c_stop(); return status; }

    status = i2c_write_byte(reg_addr);
    if (status != 0U) { i2c_stop(); return status; }

    status = i2c_write_byte(data);
    if (status != 0U) { i2c_stop(); return status; }

    i2c_stop();
    return 0U;
}

uint8_t I2C_HAL_Read(uint8_t dev_addr, uint8_t reg_addr,
                     uint8_t *buffer, uint8_t length)
{
    uint8_t status;
    uint8_t i;

    /* Phase 1: write register address */
    i2c_start();
    status = i2c_write_byte((uint8_t)(dev_addr << 1) | 0x00U);  /* W */
    if (status != 0U) { i2c_stop(); return status; }

    status = i2c_write_byte(reg_addr);
    if (status != 0U) { i2c_stop(); return status; }

    i2c_stop();

    /* Phase 2: repeated-start + read */
    i2c_start();
    status = i2c_write_byte((uint8_t)(dev_addr << 1) | 0x01U);  /* R */
    if (status != 0U) { i2c_stop(); return status; }

    for (i = 0U; i < length; i++)
    {
        uint8_t last = (uint8_t)(i == length - 1U);
        buffer[i] = i2c_read_byte(last ? 0U : 1U);
    }

    i2c_stop();
    return 0U;
}
