/******************************************************************************
 * mpu6050.h  --  MPU6050 6-Axis IMU Driver
 *
 * The MPU6050 integrates a 3-axis MEMS accelerometer and a 3-axis MEMS
 * gyroscope on a single silicon die, with an on-chip 16-bit ADC and I2C
 * interface.  This driver supports:
 *   - Device presence verification (WHO_AM_I)
 *   - Wake from sleep mode
 *   - Accelerometer full-scale selection
 *   - Simultaneous read of all six axes
 *
 * I2C address:  0x68  (AD0 pin connected to GND)
 *
 * Register map (relevant subset):
 *   Address  Name             Description
 *   -------  ---------------  ------------------------------------------
 *   0x6B     PWR_MGMT_1       Power management / sleep control
 *   0x1C     ACCEL_CONFIG     Accelerometer full-scale range
 *   0x1B     GYRO_CONFIG      Gyroscope full-scale range
 *   0x3B     ACCEL_XOUT_H     Accelerometer X-axis, high byte  (FIFO start)
 *   0x3D     ACCEL_YOUT_H     Accelerometer Y-axis, high byte
 *   0x3F     ACCEL_ZOUT_H     Accelerometer Z-axis, high byte
 *   0x41     TEMP_OUT_H       Temperature, high byte
 *   0x43     GYRO_XOUT_H      Gyroscope X-axis, high byte
 *   0x45     GYRO_YOUT_H      Gyroscope Y-axis, high byte
 *   0x47     GYRO_ZOUT_H      Gyroscope Z-axis, high byte
 *   0x75     WHO_AM_I         Device ID  (should read 0x68)
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SL)
 * Toolchain:  CC-RL  (e2 studio)
 *
 * Revision History:
 *   2026-07-07  Initial version
 ******************************************************************************/
#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

/******************************************************************************
 * Device constants
 ******************************************************************************/
#define MPU6050_I2C_ADDR            0x68U        /**< 7-bit address (AD0=GND) */
#define MPU6050_WHO_AMI_VALUE       0x68U        /**< Expected WHO_AM_I reply */
#define ACCEL_LSB_PER_G_2G          16384        /**< LSB/g at +-2g full-scale */

/******************************************************************************
 * Register addresses
 ******************************************************************************/
#define MPU6050_REG_WHO_AM_I        0x75U
#define MPU6050_REG_PWR_MGMT_1      0x6BU
#define MPU6050_REG_ACCEL_CONFIG    0x1CU
#define MPU6050_REG_GYRO_CONFIG     0x1BU
#define MPU6050_REG_ACCEL_XOUT_H    0x3BU
#define MPU6050_REG_GYRO_XOUT_H     0x43U

/******************************************************************************
 * Accelerometer full-scale options  (ACCEL_CONFIG bits 4:3)
 ******************************************************************************/
#define MPU6050_ACCEL_RANGE_2G      0x00U         /**< +-2 g  (default)    */
#define MPU6050_ACCEL_RANGE_4G      0x08U         /**< +-4 g                */
#define MPU6050_ACCEL_RANGE_8G      0x10U         /**< +-8 g                */
#define MPU6050_ACCEL_RANGE_16G     0x18U         /**< +-16 g               */

/******************************************************************************
 * Gyroscope full-scale options  (GYRO_CONFIG bits 4:3)
 ******************************************************************************/
#define MPU6050_GYRO_RANGE_250      0x00U         /**< +-250 deg/s  (default) */
#define MPU6050_GYRO_RANGE_500      0x08U         /**< +-500 deg/s            */

/******************************************************************************
 * Data structure: one complete sensor readout
 ******************************************************************************/
typedef struct {
    int16_t accel_x;    /**< Accelerometer X-axis, raw ADC count */
    int16_t accel_y;    /**< Accelerometer Y-axis, raw ADC count */
    int16_t accel_z;    /**< Accelerometer Z-axis, raw ADC count */
    int16_t gyro_x;     /**< Gyroscope X-axis, raw ADC count     */
    int16_t gyro_y;     /**< Gyroscope Y-axis, raw ADC count     */
    int16_t gyro_z;     /**< Gyroscope Z-axis, raw ADC count     */
} MPU6050_Data_t;

/******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * Initialise the MPU6050.
 *
 * Sequence:
 *   1. Verify device identity (WHO_AM_I register).
 *   2. Clear sleep bit in PWR_MGMT_1  (write 0x00 to wake).
 *   3. Set accelerometer to +-2 g full-scale.
 *   4. Set gyroscope to +-250 deg/s full-scale.
 *
 * @return 0 on success; 1 if WHO_AM_I mismatch; otherwise I2C error code.
 */
uint8_t MPU6050_Init(void);

/**
 * Read all six axes from the MPU6050 in a single I2C transaction.
 *
 * Reads 14 bytes starting at ACCEL_XOUT_H (0x3B).  Bytes 6-7 (temperature)
 * are discarded; bytes 8-13 provide gyro X/Y/Z.
 *
 * @param[out] data  Structure to populate with raw accelerometer and
 *                   gyroscope counts.
 * @return 0 on success; nonzero on I2C error.
 */
uint8_t MPU6050_ReadAll(MPU6050_Data_t *data);

#endif /* MPU6050_H */
