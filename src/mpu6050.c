/******************************************************************************
 * mpu6050.c  --  MPU6050 6-Axis IMU Driver
 *
 * All I2C operations delegate to i2c_hal.c.  The driver assumes the
 * MPU6050 is powered and the I2C bus has been initialised externally.
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SL)
 * Toolchain:  CC-RL  (e2 studio)
 *
 * Revision History:
 *   2026-07-07  Initial version
 ******************************************************************************/
#include "mpu6050.h"
#include "i2c_hal.h"

/******************************************************************************
 * Private helpers
 ******************************************************************************/

/**
 * Combine two unsigned bytes into a signed 16-bit integer.
 *
 * The MPU6050 stores 16-bit values big-endian (high byte first) in two's
 * complement form.  This function preserves the signed interpretation.
 *
 * @param[in] high  High-order byte (first in FIFO).
 * @param[in] low   Low-order byte  (second in FIFO).
 * @return  Signed 16-bit value.
 */
static int16_t combine_bytes(uint8_t high, uint8_t low)
{
    return (int16_t)((uint16_t)(high << 8) | (uint16_t)low);
}

/******************************************************************************
 * Public API
 ******************************************************************************/

uint8_t MPU6050_Init(void)
{
    uint8_t whoami;
    uint8_t status;

    /*
     * Step 1 -- Verify device presence.
     * The WHO_AM_I register (0x75) always returns 0x68 for the MPU6050.
     * A mismatch indicates the wrong device, no device, or an I2C fault.
     */
    status = I2C_HAL_Read(MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I,
                          &whoami, 1U);
    if (status != 0U)
        return status;

    if (whoami != MPU6050_WHO_AMI_VALUE)
        return 1U;

    /*
     * Step 2 -- Disable sleep mode.
     * PWR_MGMT_1[6] = 0  =>  device awake.
     * Writing 0x00 also selects the internal 8 MHz oscillator as the
     * clock source (default).
     */
    status = I2C_HAL_Write(MPU6050_I2C_ADDR,
                           MPU6050_REG_PWR_MGMT_1, 0x00U);
    if (status != 0U)
        return status;

    /*
     * Step 3 -- Set accelerometer range.
     * Default after reset is +-2 g (0x00).  We keep the default for
     * maximum resolution (16384 LSB/g).
     */
    status = I2C_HAL_Write(MPU6050_I2C_ADDR,
                           MPU6050_REG_ACCEL_CONFIG,
                           MPU6050_ACCEL_RANGE_2G);
    if (status != 0U)
        return status;

    /*
     * Step 4 -- Set gyroscope range.
     * Default after reset is +-250 deg/s (0x00).
     */
    status = I2C_HAL_Write(MPU6050_I2C_ADDR,
                           MPU6050_REG_GYRO_CONFIG,
                           MPU6050_GYRO_RANGE_250);
    if (status != 0U)
        return status;

    return 0U;
}

uint8_t MPU6050_ReadAll(MPU6050_Data_t *data)
{
    uint8_t buf[14];
    uint8_t status;

    /*
     * Burst-read 14 bytes starting at ACCEL_XOUT_H (0x3B).
     *
     * Byte layout:
     *   [0:1]   ACCEL_XOUT_H, ACCEL_XOUT_L
     *   [2:3]   ACCEL_YOUT_H, ACCEL_YOUT_L
     *   [4:5]   ACCEL_ZOUT_H, ACCEL_ZOUT_L
     *   [6:7]   TEMP_OUT_H,   TEMP_OUT_L       (skipped)
     *   [8:9]   GYRO_XOUT_H,  GYRO_XOUT_L
     *   [10:11] GYRO_YOUT_H,  GYRO_YOUT_L
     *   [12:13] GYRO_ZOUT_H,  GYRO_ZOUT_L
     */
    status = I2C_HAL_Read(MPU6050_I2C_ADDR,
                          MPU6050_REG_ACCEL_XOUT_H,
                          buf, 14U);
    if (status != 0U)
        return status;

    data->accel_x = combine_bytes(buf[0], buf[1]);
    data->accel_y = combine_bytes(buf[2], buf[3]);
    data->accel_z = combine_bytes(buf[4], buf[5]);

    data->gyro_x  = combine_bytes(buf[8], buf[9]);
    data->gyro_y  = combine_bytes(buf[10], buf[11]);
    data->gyro_z  = combine_bytes(buf[12], buf[13]);

    return 0U;
}
