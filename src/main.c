/******************************************************************************
 * main.c  --  MPU6050 + TinyML  Flat / Tilt Detector
 *
 * APPLICATION SUMMARY
 * ===================
 *
 * This program reads a 3-axis accelerometer (MPU6050) over I2C, passes
 * the raw data to a lightweight on-device classifier (ml_classifier.c),
 * and drives external LEDs to indicate the result:
 *
 *   Green LED  ->  device is FLAT  (on a horizontal surface)
 *   Red   LED  ->  device is TILTED
 *   Blue  LED  ->  system initialised OK
 *   Orange LED ->  error (blinks if MPU6050 not found)
 *
 * Status is shown via the FLAT / TILTED LEDs.
 *
 * HARDWARE CONFIGURATION
 * ======================
 *
 *   MCU:      Renesas R5F100SLAFB  (RL78/G13, 128-pin) on QB-R5F100SL-TB
 *   Clock:    20 MHz HOCO  (internal high-speed oscillator)
 *   Debugger: E2 Emulator Lite  (supplied with board)
 *   IMU:      MPU6050  (GY-521 breakout board)
 *   I2C:      Bit-bang on IICA0 pins  P60 (SCLA0) / P61 (SDAA0)
 *
 *   MPU6050    ->   QB-R5F100SL-TB
 *   -------         --------------
 *   VCC         ->   CN1  (3.3 V)
 *   GND         ->   CN1  (GND)
 *   SCL         ->   P60/SCLA0  (pin 37 -- CN1)
 *   SDA         ->   P61/SDAA0  (pin 38 -- CN1)
 *   AD0         ->   GND   (I2C address 0x68)
 *
 *   IICA0 pins (P60/P61) are used in bit-bang mode; the on-chip IICA
 *   peripheral is not configured.  This allows the physical connection
 *   to match the IICA0 header labels while keeping the driver simple.
 *
 *   External LEDs:
 *
 *     Signal      Port   Port reg   Pin mask   LED + resistor to GND
 *     ----------  -----  ---------  ---------  ---------------------
 *     FLAT      | P76  | Port 7   |  0x40    |   Green, 470R
 *     TILTED    | P77  | Port 7   |  0x80    |   Red,   470R
 *     OK        | P05  | Port 0   |  0x20    |   Blue,  470R
 *     ERROR     | P06  | Port 0   |  0x40    |   Orange,470R
 *
 *   Port 7 pins (P76, P77) are available on the expansion headers.
 *   Consult the QB-R5F100SL-TB board manual for exact connector pin numbers.
 *
 * (UART removed; debug via LEDs only)
 *
 * TARGET:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SLAFB 128-pin)
 * TOOLCHAIN:  CC-RL  (e2 studio)
 *
 * REVISION HISTORY:
 *   2026-07-07  Initial version
 ******************************************************************************/

/*
 * NOTE ON SFR ADDRESSES
 * ======================
 *
 * All port registers are accessed via their raw memory-mapped addresses
 * (e.g., *(volatile unsigned char *)0xFFF00) rather than the Code
 * Generator's bit-field structs (e.g., P0_bit.no0).  This approach:
 *
 *   - Works without running the Code Generator
 *   - Is compiler-independent (CC-RL, IAR, GCC)
 *   - Makes the hardware register map explicit
 *
 * SFR map for R5F100SLAFB (RL78/G13, 128-pin):
 *
 *   Address  Symbol  Description
 *   -------- ------- -------------------------------------------
 *   0xFFF00  P0      Port 0  data register  (OK=P05, ERROR=P06)
 *   0xFFF06  P6      Port 6  data register  (I2C SCL/SDA -- IICA0 pins)
 *   0xFFF07  P7      Port 7  data register  (FLAT, TILTED LEDs)
 *   0xFFF20  PM0     Port 0  mode register
 *   0xFFF26  PM6     Port 6  mode register
 *   0xFFF27  PM7     Port 7  mode register
 *
 */
/*
 * No iodefine.h required -- all SFR registers are accessed via their
 * raw memory-mapped addresses (see SFR table below).  This avoids any
 * dependency on the Renesas Code Generator.
 */
#include "mpu6050.h"
#include "ml_classifier.h"
#include "i2c_hal.h"

/******************************************************************************
 * LED pin mapping
 *
 * LEDs span two ports:
 *   Port 7  -- P76 (FLAT), P77 (TILTED)
 *   Port 0  -- P05 (OK),   P06 (ERROR)
 *
 * LED on = pin driven high (active-high).
 * External 470R series resistor limits current to ~5 mA at 3.3 V.
 ******************************************************************************/
/* --- SFR declarations (W0520171 suppressed -- cast is intentional for RL78 memory-mapped I/O) --- */
#pragma warning(disable:W0520171)
/* --- Port 7  (FLAT / TILTED) --- */
static volatile unsigned char * const led7_port  = (unsigned char *)0xFFF07;
static volatile unsigned char * const led7_dir   = (unsigned char *)0xFFF27;
#define LED7_PORT   (*led7_port)
#define LED7_DIR    (*led7_dir)

#define PIN_FLAT        0x40U    /**< P76, bit 6 */
#define PIN_TILTED      0x80U    /**< P77, bit 7 */

#define LED_FLAT_ON()       (LED7_PORT |=  PIN_FLAT)
#define LED_FLAT_OFF()      (LED7_PORT &= ~PIN_FLAT)
#define LED_TILTED_ON()     (LED7_PORT |=  PIN_TILTED)
#define LED_TILTED_OFF()    (LED7_PORT &= ~PIN_TILTED)

/* --- Port 0  (OK / ERROR) --- */
static volatile unsigned char * const led0_port  = (unsigned char *)0xFFF00;
static volatile unsigned char * const led0_dir   = (unsigned char *)0xFFF20;
#define LED0_PORT   (*led0_port)
#define LED0_DIR    (*led0_dir)

#define PIN_OK          0x20U    /**< P05, bit 5 */
#define PIN_ERROR       0x40U    /**< P06, bit 6 */

/* --- Clock control SFRs --- */
static volatile unsigned char * const cmc_reg = (unsigned char *)0xFFF2C;
static volatile unsigned char * const ckc_reg = (unsigned char *)0xFFF2E;
static volatile unsigned char * const csc_reg = (unsigned char *)0xFFF2F;
#define CMC   (*cmc_reg)
#define CKC   (*ckc_reg)
#define CSC   (*csc_reg)

#define LED_OK_ON()         (LED0_PORT |=  PIN_OK)
#define LED_OK_OFF()        (LED0_PORT &= ~PIN_OK)
#define LED_ERROR_ON()      (LED0_PORT |=  PIN_ERROR)
#define LED_ERROR_OFF()     (LED0_PORT &= ~PIN_ERROR)
#define LED_ERROR_TOGGLE()  (LED0_PORT ^=  PIN_ERROR)

/******************************************************************************
 * Timing constants
 ******************************************************************************/
#define SAMPLE_INTERVAL_MS  100U    /**< Sensor read period (ms)          */
#define BUSY_LOOP_CAL_1MS   650U    /**< Busy-loop iterations per ms
                                          (empirical for 16.25 MHz HOCO)    */

/******************************************************************************
 * Private:  Utility functions
 ******************************************************************************/

/**
 * Blocking millisecond delay using busy-wait loops.
 *
 * Calibrated for the RL78/G13 running at 20 MHz HOCO.
 * Accuracy is approximately +-5 %; sufficient for LED blink and
 * sensor sampling intervals but not for real-time deadlines.
 *
 * @param[in] ms  Number of milliseconds to wait.
 */
static void delay_ms(volatile uint32_t ms)
{
    volatile uint32_t i;
    volatile uint32_t j;

    for (i = 0U; i < ms; i++)
    {
        for (j = 0U; j < BUSY_LOOP_CAL_1MS; j++)
        {
            /* empty */
        }
    }
}

/******************************************************************************
 * Private:  GPIO initialisation
 ******************************************************************************/

/**
 * Configure LED GPIOs as push-pull outputs, initially low (LEDs off).
 *
 * Port 7: bits 6--7  (P76, P77)  -> FLAT / TILTED
 * Port 0: bits 2--3  (P02, P03)  -> OK / ERROR
 *
 * Direction registers (PM) are cleared to select output mode.
 * Data registers (P) are cleared to ensure all LEDs start OFF.
 */
static void Init_LEDs(void)
{
    LED7_DIR  &= ~(PIN_FLAT | PIN_TILTED);
    LED7_PORT &= ~(PIN_FLAT | PIN_TILTED);

    LED0_DIR  &= ~(PIN_OK | PIN_ERROR);
    LED0_PORT &= ~(PIN_OK | PIN_ERROR);
}

/******************************************************************************
 * Private:  Clock initialisation
 *
 * Configures the high-speed on-chip oscillator (HOCO) for the maximum
 * supported frequency (8 MHz or 16.25 MHz, depending on the specific
 * RL78/G13 variant).  The R5F100SLAFB (512 KB flash) supports 16.25 MHz
 * via CMC bit 7 (HOCODIV = 1).
 *
 * After reset the HOCO defaults to 8 MHz and the hdwinit.asm is empty,
 * so the CPU runs at 8 MHz unless this function is called.
 *
 * Registers used:
 *   CMC (0xFFF2C)  -- clock mode control
 *   CSC (0xFFF2F)  -- clock status / HOCO control
 *   CKC (0xFFF2E)  -- clock divider (fCLK = fMAIN / n)
 ******************************************************************************/

/**
 * Bump the HOCO to its highest frequency and set fCLK = fMAIN / 1.
 */
static void Clock_Init(void)
{
    /* Step 1: Stop HOCO before changing divider */
    CSC |= 0x02U;
    while ((CSC & 0x02U) == 0U);   /* wait until stopped */

    /* Step 2: Set HOCODIV = 1  (16.25 MHz on 512 KB parts) */
    CMC |= 0x80U;

    /* Step 3: Restart HOCO */
    CSC &= ~0x02U;
    while ((CSC & 0x02U) != 0U);   /* wait until stable */

    /* Step 4: Select main clock, no division */
    CSC &= ~0x80U;                  /* CSS = 0 (main clock source) */
    CKC  = 0x00U;                   /* fCLK = fMAIN / 1 */
}

/******************************************************************************
 * main  --  Application entry point
 *
 * Execution flow:
 *   0. Initialise clock (HOCO to 16.25 MHz).
 *   1. Initialise LED GPIOs.
 *   2. Blink ERROR LED once as a visual power-on self-test.
 *   3. Initialise the I2C bus and TinyML classifier.
 *   4. Initialise the MPU6050.
 *   5. If MPU6050 fails:  blink ERROR LED forever.
 *   6. Light OK LED.
 *   7. Main loop (every 100 ms):
 *        a. Read all sensor axes.
 *        b. Run classifier inference.
 *        c. Update FLAT / TILTED LEDs.
 ******************************************************************************/
void main(void)
{
    /* -- Local variables -- */
    MPU6050_Data_t    sensor_data;
    ClassifierResult_t ml_result;
    uint8_t           mpu_ok;

    /* -- Step 0: Configure clock to highest HOCO frequency -- */
    Clock_Init();

    /* -- Step 1: Initialise LED hardware -- */
    Init_LEDs();

    /* -- Step 2: Power-on self-test (blink ERROR LED once) -- */
    LED_ERROR_ON();
    delay_ms(200U);
    LED_ERROR_OFF();
    delay_ms(200U);

    /* -- Step 3: Initialise I2C bus and classifier -- */
    I2C_HAL_Init();
    ML_Classifier_Init();

    /* Allow MPU6050 power supply to stabilise */
    delay_ms(100U);

    /* -- Step 4: Initialise MPU6050 -- */
    mpu_ok = MPU6050_Init();

    /* -- Step 5: Handle init failure -- */
    if (mpu_ok != 0U)
    {
        LED_ERROR_ON();
        while (1U)
        {
            LED_ERROR_TOGGLE();
            delay_ms(500U);
        }
    }

    /* -- Step 6: System ready -- */
    LED_OK_ON();
    delay_ms(500U);

    /* -- Step 7: Main acquisition and classification loop -- */
    while (1U)
    {
        /* 7a. Read sensor */
        if (MPU6050_ReadAll(&sensor_data) == 0U)
        {
            /* 7b. Run inference */
            ML_Classifier_Predict(
                sensor_data.accel_x,
                sensor_data.accel_y,
                sensor_data.accel_z,
                &ml_result
            );

            /* 7c. Drive status LEDs */
            if (ml_result.prediction == CLASS_FLAT)
            {
                LED_FLAT_ON();
                LED_TILTED_OFF();
            }
            else
            {
                LED_FLAT_OFF();
                LED_TILTED_ON();
            }
        }
        else
        {
            /* I2C read failure -- toggle error LED */
            LED_ERROR_TOGGLE();
        }

        /* Wait for next sample period */
        delay_ms(SAMPLE_INTERVAL_MS);
    }
}
