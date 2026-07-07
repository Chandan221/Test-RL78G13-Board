/******************************************************************************
 * ml_classifier.h  --  TinyML Flat / Tilt Classifier
 *
 * A lightweight, physics-informed binary classifier that determines whether
 * an accelerometer-equipped device is resting on a flat surface or is
 * tilted.  The algorithm is described in detail in ml_classifier.c.
 *
 * Inference requires no pre-trained model file, no floating-point library
 * linkage, and under 200 bytes of stack.  It is suitable for sub-10 kB
 * MCU targets such as the RL78/G13.
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SL)
 * Toolchain:  CC-RL  (e2 studio)
 *
 * Revision History:
 *   2026-07-07  Initial version
 ******************************************************************************/
#ifndef ML_CLASSIFIER_H
#define ML_CLASSIFIER_H

#include <stdint.h>

/******************************************************************************
 * Classification output
 ******************************************************************************/

/** Enumerated prediction labels. */
typedef enum {
    CLASS_FLAT   = 0,    /**< Device is on a flat / horizontal surface.  */
    CLASS_TILTED = 1     /**< Device is tilted away from horizontal.     */
} TiltClass_t;

/** Structured inference result. */
typedef struct {
    /* -- raw features (g-force) -- */
    float accel_x_g;        /**< X-axis acceleration in g.               */
    float accel_y_g;        /**< Y-axis acceleration in g.               */
    float accel_z_g;        /**< Z-axis acceleration in g.               */

    /* -- derived features -- */
    float magnitude_g;      /**< Total acceleration vector magnitude.     */
    float pitch_deg;        /**< Pitch angle in degrees.                  */
    float roll_deg;         /**< Roll angle in degrees.                   */
    float tilt_angle_deg;   /**< Deviation from horizontal (0 = flat).   */

    /* -- decision -- */
    TiltClass_t prediction; /**< CLASS_FLAT or CLASS_TILTED.             */
    float confidence;       /**< 0.0 (uncertain) to 1.0 (certain).      */
} ClassifierResult_t;

/******************************************************************************
 * Public API
 ******************************************************************************/

/**
 * Initialise the classifier.
 *
 * Currently a no-op; reserved for future model loading or state setup.
 */
void ML_Classifier_Init(void);

/**
 * Run inference on accelerometer data.
 *
 * Converts raw ADC counts to g-force, computes the tilt angle from the
 * gravity vector, and classifies the orientation as FLAT or TILTED.
 *
 * @param[in]  accel_x  Raw X-axis accelerometer count (int16_t).
 * @param[in]  accel_y  Raw Y-axis accelerometer count (int16_t).
 * @param[in]  accel_z  Raw Z-axis accelerometer count (int16_t).
 * @param[out] result   Structure receiving the full inference output
 *                      (features, angles, label, confidence).
 */
void ML_Classifier_Predict(int16_t accel_x,
                           int16_t accel_y,
                           int16_t accel_z,
                           ClassifierResult_t *result);

#endif /* ML_CLASSIFIER_H */
