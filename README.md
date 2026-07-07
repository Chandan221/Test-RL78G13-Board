# MPU6050 + TinyML Flat/Tilt Detector for Renesas QB-R5F100SL-TB

## Overview

This project interfaces an **MPU6050 6-axis IMU sensor** with a **Renesas QB-R5F100SL-TB** evaluation board (RL78/G13 microcontroller) and implements a **TinyML classifier** that determines whether the device is resting on a **flat surface** or is **tilted**. The classification uses accelerometer data processed through a lightweight on-device inference engine that computes pitch, roll, and tilt angles with confidence scoring.

The entire codebase accesses hardware registers through raw memory-mapped SFR 
addresses -- **no Code Generator dependency**. All I2C and GPIO operations work with the CC-RL compiler alone, 
using an empty e2 studio project.

### Hardware Requirements

- **Renesas QB-R5F100SL-TB** evaluation board (RL78/G13, R5F100SL)
- **E2 Emulator Lite** debugger (included with QB-R5F100SL-TB kit)
- **MPU6050** module (GY-521 breakout board)
- Breadboard and jumper wires (female-to-female)
- USB mini-B cable (for E2 Lite)
- (UART removed; debug via LEDs only)
- 4x LEDs (green, red, blue, orange) + 4x 470R resistors

### Pin Connections

```
MPU6050    ->   QB-R5F100SL-TB
-------         --------------
VCC         ->   CN1  (3.3 V)
GND         ->   CN1  (GND)
SCL         ->   P60/SCLA0  (pin 37 -- CN1)
SDA         ->   P61/SDAA0  (pin 38 -- CN1)
AD0         ->   GND   (I2C address 0x68)

Note: I2C is bit-bang on the IICA0-dedicated pins (P60/P61).
The on-chip IICA0 peripheral is NOT configured; only the GPIO
port registers are used.  This keeps the driver simple while
using the physical pins labelled IICA0 on the board headers.

External LEDs:

  Signal   Port   Pin mask   Port reg   LED colour   Resistor
  -------- -----  ---------  ---------  ------------  --------
  FLAT   | P76  |  0x40     | Port 7   | Green       | 470R
  TILTED | P77  |  0x80     | Port 7   | Red         | 470R
  OK     | P05  |  0x20     | Port 0   | Blue        | 470R
  ERROR  | P06  |  0x40     | Port 0   | Orange      | 470R
  GND    |  --  |   --      |   --     | cathode     |

  Connect:  LED anode -> resistor -> port pin
            LED cathode -> GND

  Port 7 pins (P76, P77) are available on the J2 expansion header;
  see the QB-R5F100SL-TB board manual for exact J2 pin numbers.

(UART removed; debug via LEDs only)
```

---

## Project File Structure

```
flat_or_tilt_detection/
├── README.md                   # This file -- full setup guide
└── src/
    ├── main.c                  # Application entry point, LED GPIO
    ├── i2c_hal.h               # I2C bit-bang HAL -- API header
    ├── i2c_hal.c               # I2C bit-bang on P40/P41 (SFR-addressed)
    ├── mpu6050.h               # MPU6050 register map / device constants
    ├── mpu6050.c               # MPU6050 init + burst read driver
    ├── ml_classifier.h         # TinyML classifier -- result struct + API
    └── ml_classifier.c         # Physics-informed tilt inference engine
```

---

## Step-by-Step e2 Studio Project Setup

### Step 1: Install Required Software

1. **Download and install Renesas e2 studio** from the Renesas website.
2. **Install the RL78/G13 toolchain** (CC-RL compiler) during or after e2 studio installation.
3. **Install the E2 Emulator Lite driver** (if not automatically installed with e2 studio).
4. **Install Renesas Flash Programmer** (optional, for standalone flashing).

### Step 2: Create a New RL78 Project in e2 studio

1. Launch **e2 studio**.
2. Go to **File -> New -> C/C++ Project -> Renesas RL78**.
3. Select **RL78 C/C++ Project** and click **Next**.
4. Enter a project name: `flat_or_tilt_detection`.
5. Click **Next** twice to reach the **Device Configuration** page.
6. Set the following:
   - **Device**: `R5F100SL`
   - **Toolchain**: `CC-RL` (v1.x or later)
   - **Debugger**: `E2 Emulator Lite`
7. Click **Next**.
8. Select **"Create an empty project"**.
   > No Code Generator needed -- all register addresses are hardcoded in the source.
9. Click **Finish**.

### Step 3: Add Source Files to the Project

> **Note on the auto-generated file:** e2 studio creates a `.c` file named after your
> project (e.g. `flat_or_tilt_detection.c`) inside `src/`.  You will replace its
> contents with our `main.c` code (see Step 3a).  Our `main.c` is provided as a
> reference; you do NOT import it into the project.

1. Right-click the project in **Project Explorer** -> **Import** -> **General -> File System**.
2. Click **Next**, then **Browse**, and select the `src/` folder from this repository.
3. Select these **six files** (do NOT select `main.c`):
   - `i2c_hal.c`, `i2c_hal.h`
   - `mpu6050.c`, `mpu6050.h`
   - `ml_classifier.c`, `ml_classifier.h`
4. Ensure **"Create top-level folder"** is **unchecked** (files should import into the project root).
5. Click **Finish**.

#### Step 3a: Replace the auto-generated project file

1. In the **Project Explorer**, open `src/flat_or_tilt_detection.c`
   (the file that e2 studio created for you).
2. Delete all its contents and paste the entire contents of our `src/main.c`
   into it.
3. Save (`Ctrl+S`).

   > If you accidentally also imported `main.c`, you will get a
   > `E0562300:Duplicate symbol "_main"` linker error.  Fix it by right-clicking
   > `src/main.c` -> **Delete** or **Exclude from Build**.

### Step 4: Configure the Build Settings

1. Right-click the project in **Project Explorer** and select **Properties**.
2. Navigate to **C/C++ Build -> Settings -> Compiler -> Source**.
3. Set **C language standard** to **C99** (or later).
4. In the same **Source** tab, locate **Include file search path** and add the
   project root directory (where the `.h` files reside).  Click the folder icon,
   select **"Workspace"** and browse to your project folder, then click OK.
   If the `.h` files are inside a `src/` subfolder, add that path instead.
5. Click **Apply and Close**.
   > No math library linkage is required -- the classifier includes its own
   > `sqrt()` and `atan2()` approximations.

### Step 5: Configure the Debugger (E2 Emulator Lite)

1. Right-click the project -> **Debug As** -> **Debug Configurations**.
2. Select **Renesas GDB Hardware Debugging** and click the **New** icon.
3. In the **Main** tab:
   - **Project**: Your project name.
   - **C/C++ Application**: The built `.elf` file (auto-filled after first build).
4. In the **Debugger** tab:
   - **Debug hardware**: `E2 Emulator Lite (RL78)`.
   - **Target device**: `R5F100SL`.
   - **Connection**: Select the USB-serial number of your E2 Lite (auto-detected).
   - **Supply voltage**: `3.3 V`.
   - **Power**: Check **"Target power supplied by emulator"** (E2 Lite powers the board).
5. In the **Debug Tool Settings** tab:
   - **Clock**: `HOCO` (High-speed on-chip oscillator) at `20 MHz`.
   - **Mode**: `Single-chip mode`.
6. Click **Apply** and **Close**.

### Step 6: Build the Project

1. Click **Project -> Build All** (or press **Ctrl+B**).
2. Check the **Console** view.  There should be zero errors.
3. If any SFR address is flagged as an invalid memory region, verify the
   addresses in the code match the **R5F100SL** register map (not a
   different RL78 variant).

### Step 7: Connect the Hardware

1. Connect the **E2 Emulator Lite** to the QB-R5F100SL-TB board via the
   **14-pin header (J3)**.  Align the red wire mark on the ribbon cable
   with pin 1 (marked on the board silk-screen).
2. Connect the **USB mini-B** cable from the E2 Lite to your PC.
3. Wire the **MPU6050** and **LEDs** according to the pin table above.
4. The board is now powered by the E2 Lite (no external power supply needed).

### Step 8: Flash and Debug

1. Right-click the project -> **Debug As** -> **Renesas GDB Hardware Debugging**.
2. e2 studio will:
   - Build the project.
   - Flash the binary to the RL78 via E2 Lite.
   - Transition to the **Debug perspective**.
3. In the Debug perspective, click **Resume** (F8) to start the application.
4. Observe the LEDs:

   | LED      | Port | Colour | Meaning                              |
   |----------|------|--------|--------------------------------------|
   | FLAT     | P76  | Green  | Device is FLAT (tilt < 15 deg)       |
   | TILTED   | P77  | Red    | Device is TILTED (tilt >= 15 deg)    |
   | OK       | P05  | Blue   | System initialised OK (steady ON)    |
   | ERROR    | P06  | Orange | Blinking = MPU6050 init failure      |

   At startup:
   - ERROR blinks once (POST), then off.
   - OK lights up = MPU6050 found, system ready.
   - If ERROR continues blinking -> check I2C wiring.

### Step 9: Verify I2C Communication

1. The ERROR LED behaviour indicates I2C health:
   - **Off** after startup → MPU6050 initialised OK.
   - **Blinking every 100ms** → I2C read failures (check wiring/pull-ups).
   - **On solid** → MPU6050 init failed (check AD0 = GND, VCC = 3.3V).
4. Every ~1 second, a frame like this appears:

   ```
   === MPU6050 + TinyML Flat/Tilt Detector ===
   Accel (raw): X=+00123 Y=-00045 Z=+16320
   Accel (g): X=0.012 Y=-0.075 Z=0.997
   Magnitude: 1.002 g
   Pitch: 0.7 deg  Roll: -4.3 deg  Tilt: 2.1 deg
   Prediction: FLAT  (Confidence: 86%)
   ```

---

## TinyML Algorithm Details

### How the Classifier Works

The TinyML engine implements a **physics-informed tilt classifier** that requires no prior training data:

```
1. Convert raw ADC counts to g-force:
   ax_g = accel_x / 16384.0    (+-2 g full scale)
   ay_g = accel_y / 16384.0
   az_g = accel_z / 16384.0

2. Compute the total acceleration magnitude:
   mag = sqrt(ax_g^2 + ay_g^2 + az_g^2)
   Expected ~1.0 g when stationary (gravity only).

3. Compute pitch and roll angles:
   pitch = atan2(ax_g, sqrt(ay_g^2 + az_g^2))
   roll  = atan2(ay_g, sqrt(ax_g^2 + az_g^2))

4. Compute deviation from horizontal (tilt angle):
   tilt = 90 - asin(|az_g| / mag)

5. Decision rule:
   tilt < 15  ->  FLAT     (confidence: 1 - tilt/15)
   tilt >= 15 ->  TILTED   (confidence: (tilt-15) / 75)
```

### Custom Math Library

The classifier includes **self-contained `sqrt()`, `atan2()`, and `asin()`
approximations** that do not require linking `<math.h>`:

- **sqrt**: Newton-Raphson iteration (10 iterations, < 0.05 % error)
- **atan2**: Third-order polynomial on [-1, 1] with quadrant correction
  (< 0.3 degree error)

This keeps the code footprint under **8 KB of flash** and under **1 KB of RAM**.

### Extending the Model

To replace the rule-based classifier with a trained model:

1. Collect labelled accelerometer data in several orientations.
2. Train a model in Python (scikit-learn decision tree / SVM, or
   TensorFlow Lite Micro neural network).
3. Export parameters as C arrays (coefficients, thresholds, weights).
4. Replace the `ML_Classifier_Predict()` body in `ml_classifier.c` with
   the model's inference code.

---

## SFR Address Reference

All peripheral registers are accessed via their raw memory-mapped addresses.
This table documents every address used in the code:

| Address  | Symbol  | Used in          | Description                              |
|----------|---------|------------------|------------------------------------------|
| 0xFFF00  | P0      | `main.c`         | Port 0 data register (OK/ERROR LEDs)     |
| 0xFFF20  | PM0     | `main.c`         | Port 0 mode register                     |
| 0xFFF06  | P6      | `i2c_hal.c`      | Port 6 data register (I2C SCL/SDA)       |
| 0xFFF26  | PM6     | `i2c_hal.c`      | Port 6 mode register                     |
| 0xFFF07  | P7      | `main.c`         | Port 7 data register (FLAT/TILTED LEDs)  |
| 0xFFF27  | PM7     | `main.c`         | Port 7 mode register                     |
| 0xFFF10  | SDR00L  | `main.c`         | SAU0 serial data / baud rate (low byte)  |
| 0xFFF11  | SDR00H  | `main.c`         | SAU0 serial data / baud rate (high byte) |
| 0xFFF12  | SCR00H  | `main.c`         | SAU0 control (TXE, RXE)                  |
| 0xFFF14  | SSR00L  | `main.c`         | SAU0 status (TSF flag)                   |
| 0xFFF18  | ST0     | `main.c`         | SAU0 channel 0 start trigger             |
| _removed_ | _UART_  | _(not used)_     | _bit-bang UART code removed_             |

> **Note for different RL78 variants**: If porting to a different package or
> derivative, adjust the port and SFR addresses per the device's hardware
> manual.

---

## Troubleshooting

| Symptom | Likely Cause | Solution |
|---|---|---|
| Orange LED (P06) blinks continuously | MPU6050 not found | Check I2C wiring 
(SCL/SDA), verify VCC=3.3V, check AD0 pull-down to GND |
| No LEDs light up | Power issue | Ensure the E2 Lite is configured to supply 3.3 V (Step 5, item 4) |
| Wrong or no LED behaviour | Pin assignment mismatch | Verify FLAT/TILTED on 
P76/P77, OK/ERROR on P05/P06, cathodes to GND |
| Build error: undefined symbol `something` | SFR address conflict | Verify you selected `R5F100SL` device, not a different RL78 variant |
| (UART removed) | | |
| Random FLAT/TILTED flipping | Noisy readings | Add a moving-average filter over 3-5 samples before calling classifier |
| ERROR LED blinks briefly then OK lights up | Normal | That is the intentional power-on self-test (200 ms blink) |

### I2C Bus Verification

To manually check I2C communication, add this code to `main.c` after
`I2C_HAL_Init()` and step through with the debugger:

```c
uint8_t whoami;
if (I2C_HAL_Read(0x68, 0x75, &whoami, 1) == 0)
{
    if (whoami == 0x68)
        LED_OK_ON();     /* MPU6050 responded correctly */
    else
        LED_ERROR_ON();  /* Wrong device ID */
}
else
{
    LED_ERROR_ON();      /* No ACK -- wiring or power issue */
}
```

---

## Technical Specifications

| Parameter | Value |
|---|---|
| MCU | R5F100SLAFB (RL78/G13, 16-bit, 128-pin) |
| Clock | 20 MHz HOCO (internal) |
| Flash / RAM | 32 KB / 2 KB (est. usage: 8 KB / 1 KB) |
| IMU | MPU6050 (3-axis accel + 3-axis gyro) |
| Accel Range | +-2 g (configurable up to +-16 g) |
| Accel Resolution | 16384 LSB/g at +-2 g |
| I2C Speed | ~100 kHz (bit-bang, standard-mode) |
| Sampling Rate | 10 Hz |
| (UART removed) | N/A |
| Classifier Type | Physics-informed threshold (15 deg) |
| Math Library | Custom (no <math.h> required) |
| Power Supply | 3.3 V from E2 Lite emulator |
| Board Current | ~30 mA (board + MPU6050 + 4 LEDs) |

---

## License

This project is provided as reference software for educational and prototyping purposes.
No warranty or liability is assumed for use in production systems.
