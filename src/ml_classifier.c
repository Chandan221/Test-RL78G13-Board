/******************************************************************************
 * ml_classifier.c  --  TinyML Flat / Tilt Classifier
 *
 * ALGORITHM OVERVIEW
 * ==================
 *
 * The classifier exploits a simple physical fact: a stationary
 * accelerometer measures only the gravity vector (1 g downward).
 *
 *   1. Convert raw ADC counts to g-force:
 *
 *        ax_g = accel_x / 16384.0    (+-2 g scale => 16384 LSB/g)
 *        ay_g = accel_y / 16384.0
 *        az_g = accel_z / 16384.0
 *
 *   2. Compute the magnitude (should be ~1.0 g at rest):
 *
 *        mag = sqrt(ax_g^2 + ay_g^2 + az_g^2)
 *
 *   3. Compute pitch and roll angles:
 *
 *        pitch = atan2(ax_g, sqrt(ay_g^2 + az_g^2))
 *        roll  = atan2(ay_g, sqrt(ax_g^2 + az_g^2))
 *
 *   4. Compute the tilt angle -- the angle between the Z-axis and
 *      vertical (0 deg = Z points straight up, 90 deg = on its side):
 *
 *        tilt = 90 - asin(|az_g| / mag)
 *
 *   5. Decision rule:
 *        tilt < TILT_THRESHOLD_DEG  ->  FLAT
 *        tilt >= TILT_THRESHOLD_DEG ->  TILTED
 *
 *   6. Confidence:
 *        FLAT:   1.0 - (tilt / threshold)     [1 = perfectly flat]
 *        TILTED: (tilt - threshold) / (90 - threshold)  [1 = vertical]
 *
 * MATH ROUTINES
 * =============
 *
 * sqrt() and atan2() are implemented as self-contained fixed-iteration
 * approximations to avoid linking the full <math.h> library.  This
 * reduces the code footprint by several kilobytes.
 *
 *   - sqrt:  Newton-Raphson, 10 iterations, < 0.1 % error.
 *   - atan2: Polynomial approximation of atan(r) on [-1, 1] with
 *            quadrant correction; error < 0.3 deg.
 *
 * MODEL CUSTOMISATION
 * ===================
 *
 * To replace this rule-based model with a trained model (decision tree,
 * SVM, tiny NN):
 *
 *   a) Collect labelled data via UART (see main.c).
 *   b) Train in Python (scikit-learn / TensorFlow Lite Micro).
 *   c) Export parameters as C arrays.
 *   d) Replace ML_Classifier_Predict() body with model inference.
 *
 * Target:     Renesas QB-R5F100SL-TB  (RL78/G13, R5F100SL)
 * Toolchain:  CC-RL  (e2 studio)
 *
 * Revision History:
 *   2026-07-07  Initial version
 ******************************************************************************/
#include "ml_classifier.h"

/******************************************************************************
 * Constants
 ******************************************************************************/

/**
 * Decision threshold in degrees.
 *
 * If the computed tilt angle is less than this value, the device is
 * classified as FLAT; otherwise TILTED.  15 degrees provides a good
 * balance between noise immunity and sensitivity for most applications.
 */
#define TILT_THRESHOLD_DEG     15.0f

/**
 * LSB-per-g value for the +-2 g accelerometer full-scale range.
 */
#define GRAVITY_LSB            16384.0f

/******************************************************************************
 * Private:  Lightweight math routines
 *
 * These are provided to avoid pulling in the standard <math.h> library,
 * which adds ~4 kB of code on CC-RL.  If your application already links
 * math.h, you may substitute these with the standard library versions.
 ******************************************************************************/

/**
 * Absolute value for float.
 */
static float abs_f(float val)
{
    return (val < 0.0f) ? -val : val;
}

/**
 * Square root via Newton-Raphson iteration.
 *
 * Convergence criterion: |x_{n+1} - x_n| < 0.001.
 * Typical error: < 0.05 % for inputs in [0.1, 10].
 *
 * @param[in] val  Input value (must be >= 0).
 * @return Approximate square root.
 */
static float sqrt_f(float val)
{
    if (val <= 0.0f)
        return 0.0f;

    float guess = val * 0.5f;
    int   i;

    for (i = 0; i < 10; i++)
    {
        float next = (guess + val / guess) * 0.5f;
        if (abs_f(next - guess) < 0.001f)
            break;
        guess = next;
    }
    return guess;
}

/**
 * Four-quadrant arctan approximation.
 *
 * Implementation:
 *   This is a fast polynomial approximation of atan(r) on the interval
 *   [-1, 1]:
 *
 *     atan(r) ~ 0.1963 * r^3 - 0.9817 * r + pi/4
 *
 *   Quadrant expansion handles all (x, y) pairs.
 *
 * Maximum absolute error: < 0.3 degrees across all quadrants.
 *
 * @param[in] y  Y-axis coordinate.
 * @param[in] x  X-axis coordinate.
 * @return Angle in radians in [-pi, pi].
 */
static float atan2_f(float y, float x)
{
    float abs_y = abs_f(y) + 1e-10f;    /* avoid division by zero */
    float r, angle;

    if (x >= 0.0f)
    {
        /* Quadrant I / IV */
        r     = (x - abs_y) / (x + abs_y);
        angle = 0.1963f * r * r * r - 0.9817f * r + 0.785398163f;
    }
    else
    {
        /* Quadrant II / III */
        r     = (x + abs_y) / (abs_y - x);
        angle = 0.1963f * r * r * r - 0.9817f * r + 0.785398163f;
        angle = 1.570796327f + angle;
    }

    if (y < 0.0f)
        angle = -angle;

    return angle;
}

/**
 * Arcsine via atan2 identity.
 *
 *   asin(t) = atan2(t, sqrt(1 - t^2))
 *
 * Input clamped to [-1, 1].
 */
static float asin_f(float val)
{
    if (val > 1.0f)  val = 1.0f;
    if (val < -1.0f) val = -1.0f;
    return atan2_f(val, sqrt_f(1.0f - val * val));
}

/**
 * Convert radians to degrees.
 */
static float rad_to_deg(float rad)
{
    return rad * 57.295779513f;         /* 180 / pi */
}

/******************************************************************************
 * Public API
 ******************************************************************************/

void ML_Classifier_Init(void)
{
    /*
     * Reserved for future use:  model parameter loading, state reset, etc.
     */
}

void ML_Classifier_Predict(int16_t accel_x,
                           int16_t accel_y,
                           int16_t accel_z,
                           ClassifierResult_t *result)
{
    /*
     * Step 1 -- Convert raw ADC counts to g-force.
     */
    float ax = (float)accel_x / GRAVITY_LSB;
    float ay = (float)accel_y / GRAVITY_LSB;
    float az = (float)accel_z / GRAVITY_LSB;

    result->accel_x_g = ax;
    result->accel_y_g = ay;
    result->accel_z_g = az;

    /*
     * Step 2 -- Total acceleration magnitude.
     * For a stationary device this should be ~1.0 g.  Values significantly
     * different from 1.0 g indicate motion, which may reduce confidence.
     */
    result->magnitude_g = sqrt_f(ax * ax + ay * ay + az * az);

    /*
     * Step 3 -- Pitch and roll angles.
     *   pitch: rotation about Y-axis (positive = lifted X side up)
     *   roll:  rotation about X-axis (positive = lifted Y side up)
     */
    result->pitch_deg = rad_to_deg(
                            atan2_f(ax, sqrt_f(ay * ay + az * az)));
    result->roll_deg  = rad_to_deg(
                            atan2_f(ay, sqrt_f(ax * ax + az * az)));

    /*
     * Step 4 -- Tilt angle (deviation from horizontal).
     * When flat, az is near 1 g and tilt approaches 0 degrees.
     * When vertical (on edge), az is near 0 g and tilt approaches 90 deg.
     */
    float abs_az = abs_f(az);
    float tilt_rad = asin_f(abs_az / result->magnitude_g);
    result->tilt_angle_deg = 90.0f - rad_to_deg(tilt_rad);

    /*
     * Step 5 -- Decision and confidence.
     */
    if (result->tilt_angle_deg < TILT_THRESHOLD_DEG)
    {
        result->prediction = CLASS_FLAT;
        result->confidence = 1.0f - (result->tilt_angle_deg
                                     / TILT_THRESHOLD_DEG);
        if (result->confidence < 0.0f)
            result->confidence = 0.0f;
    }
    else
    {
        result->prediction = CLASS_TILTED;
        float excess = result->tilt_angle_deg - TILT_THRESHOLD_DEG;
        result->confidence = excess / (90.0f - TILT_THRESHOLD_DEG);
        if (result->confidence > 1.0f)
            result->confidence = 1.0f;
    }
}
