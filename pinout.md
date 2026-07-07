# QB-R5F100SL-TB (RL78/G13 R5F100SLAFB) Pinout

**Board:** QB-R5F100SL-TB Target Board
**MCU:** R5F100SLAFB (RL78/G13, 512 KB Flash, 32 KB RAM)
**Package:** 128-pin LFQFP (14×20 mm, 0.5 mm pitch), PLQP0128KD-A

> Sources:
> - RL78/G13(512KB FLASH) Target board QB-R5F100SL-TB User's Manual Rev.2.00 (R20UT0827XJ)
> - RL78/G13 User's Manual: Hardware R01UH0146 Rev.3.80
> - Extracted from package pinout diagram in R01UH0146 §1.3.14

---

## Board Components

| Component | MCU Connection  | Notes                    |
|-----------|-----------------|--------------------------|
| LED1      | P76             | Yellow, active low       |
| LED2      | P77             | Yellow, active low       |
| SW1       | P137 (INTP0)    | Push button, active low  |
| Power LED | VDD             | Red (LED3)               |
| Main OSC  | X1/P121, X2/P122 | 20 MHz resonator        |
| Sub OSC   | XT1/P123, XT2/P124 | 32.768 kHz resonator   |

---

## Debug/Programming Connector (FP1) — 14-pin, 2.54 mm pitch

For use with E1, E2, or E2 Lite emulators.

| Pin | Signal   | Notes                             |
|-----|----------|-----------------------------------|
| 1   | VDD      | Target power                      |
| 2   | GND      | Ground                            |
| 3   | GND      | Ground                            |
| 4   | –        | Reserved                          |
| 5   | –        | Reserved                          |
| 6   | –        | Reserved                          |
| 7   | –        | Reserved                          |
| 8   | –        | Reserved                          |
| 9   | –        | Reserved                          |
| 10  | –        | Reserved                          |
| 11  | RESET     | Reset signal (active low)        |
| 12  | GND      | Ground                            |
| 13  | TOOL0    | Flash/debug data I/O (P40)        |
| 14  | GND      | Ground                            |

---

## Peripheral Board Connectors (CN1, CN2) — 40-pin × 2, 2.54 mm pitch

Unpopulated solder-pad headers that bring out MCU signals. Consult the board
silkscreen and the user's manual schematic for exact routing.

---

## Complete MCU Pinout — 128-pin LFQFP

Pin numbering: counterclockwise from pin 1 (bottom-left corner, marked with a dot).

### Bottom Edge (pins 1–38, left to right)

| Pin | Signal                        | Alternate Functions                           |
|-----|-------------------------------|-----------------------------------------------|
| 1   | P142/SCK30/SCL30              | Serial clock 30 / I2C clock 30                |
| 2   | P141/PCLBUZ1/INTP7            | Programmable clock/buzzer 1 / ext. interrupt 7|
| 3   | P140/PCLBUZ0/INTP6            | Programmable clock/buzzer 0 / ext. interrupt 6|
| 4   | P120/ANI19                    | ADC input 19                                  |
| 5   | P37/ANI21                     | ADC input 21                                  |
| 6   | P36/ANI22                     | ADC input 22                                  |
| 7   | P35/ANI23                     | ADC input 23                                  |
| 8   | P34                           | General-purpose I/O                           |
| 9   | P33                           | General-purpose I/O                           |
| 10  | P32                           | General-purpose I/O                           |
| 11  | P106/TI17/TO17                | Timer I/O 17                                  |
| 12  | P105/TI16/TO16                | Timer I/O 16                                  |
| 13  | P104/TI15/TO15                | Timer I/O 15                                  |
| 14  | P103/TI14/TO14                | Timer I/O 14                                  |
| 15  | P47/INTP2                     | External interrupt 2                          |
| 16  | P46/INTP1/TI05/TO05           | Ext. interrupt 1 / Timer I/O 5                |
| 17  | P45/SO01                      | Serial output 01                              |
| 18  | P44/SI01/SDA01                | Serial input 01 / I2C data 01                 |
| 19  | P43/SCK01/SCL01               | Serial clock 01 / I2C clock 01                |
| 20  | P42/TI04/TO04                 | Timer I/O 4                                   |
| 21  | P41                           | General-purpose I/O                           |
| 22  | P40/TOOL0                     | Flash/debug data I/O                          |
| 23  | P127                          | General-purpose I/O                           |
| 24  | P126                          | General-purpose I/O                           |
| 25  | P125                          | General-purpose I/O                           |
| 26  | RESET                         | Reset (active low, input only)                |
| 27  | P124/XT2/EXCLKS               | Subsystem crystal 2 / external clock input    |
| 28  | P123/XT1                      | Subsystem crystal 1                           |
| 29  | P137/INTP0                    | External interrupt 0                          |
| 30  | P122/X2/EXCLK                 | Main crystal 2 / external clock input         |
| 31  | P121/X1                       | Main crystal 1                                |
| 32  | REGC                          | Regulator capacitance (0.47–1 µF to VSS)     |
| 33  | VSS                           | Ground                                        |
| 34  | EVSS0                         | Ground for port                               |
| 35  | VDD                           | Power supply                                  |
| 36  | EVDD0                         | Power supply for port                         |
| 37  | P60/SCLA0                     | I2C clock 0 (IICA0)                           |
| 38  | P61/SDAA0                     | I2C data 0 (IICA0)                            |

### Right Edge (pins 39–64, bottom to top)

| Pin | Signal                        | Alternate Functions                           |
|-----|-------------------------------|-----------------------------------------------|
| 39  | P62/SCLA1                     | I2C clock 1 (IICA1)                           |
| 40  | P63/SDAA1                     | I2C data 1 (IICA1)                            |
| 41  | P31/TI03/TO03/INTP4/(PCLBUZ0) | Timer I/O 3 / ext. interrupt 4 / buzzer 0     |
| 42  | P64/TI10/TO10                 | Timer I/O 10                                  |
| 43  | P65/TI11/TO11                 | Timer I/O 11                                  |
| 44  | P66/TI12/TO12                 | Timer I/O 12                                  |
| 45  | P67/TI13/TO13                 | Timer I/O 13                                  |
| 46  | P77/KR7/INTP11/(TxD2)         | Key return 7 / ext. interrupt 11 / UART Tx 2  |
| 47  | P76/KR6/INTP10/(RxD2)         | Key return 6 / ext. interrupt 10 / UART Rx 2  |
| 48  | P75/KR5/INTP9                 | Key return 5 / ext. interrupt 9               |
| 49  | P74/KR4/INTP8                 | Key return 4 / ext. interrupt 8               |
| 50  | P73/KR3                       | Key return 3                                  |
| 51  | P72/KR2/SO21                  | Key return 2 / serial output 21               |
| 52  | P71/KR1/SI21/SDA21            | Key return 1 / serial input 21 / I2C data 21  |
| 53  | P70/KR0/SCK21/SCL21           | Key return 0 / serial clock 21 / I2C clock 21 |
| 54  | P06                           | General-purpose I/O                           |
| 55  | P05                           | General-purpose I/O                           |
| 56  | EVSS1                         | Ground for port                               |
| 57  | EVDD1                         | Power supply for port                         |
| 58  | P80/(SCK10)/(SCL10)           | Serial clock 10 / I2C clock 10 (PIOR)         |
| 59  | P81/(SI10)/(RxD1)/(SDA10)     | Serial in 10 / UART Rx 1 / I2C data 10 (PIOR) |
| 60  | P82/(SO10)/(TxD1)             | Serial out 10 / UART Tx 1 (PIOR)              |
| 61  | P83                           | General-purpose I/O                           |
| 62  | P84/(INTP6)                   | External interrupt 6 (PIOR)                   |
| 63  | P85/(INTP7)                   | External interrupt 7 (PIOR)                   |
| 64  | P86/(INTP8)                   | External interrupt 8 (PIOR)                   |

> Note: (PIOR) = function assignable via Peripheral I/O Redirection Register.

### Top Edge (pins 65–102, right to left)

| Pin | Signal                        | Alternate Functions                           |
|-----|-------------------------------|-----------------------------------------------|
| 65  | P87/(INTP9)                   | External interrupt 9 (PIOR)                   |
| 66  | P30/INTP3/RTC1HZ              | Ext. interrupt 3 / RTC 1 Hz output            |
| 67  | P50                           | General-purpose I/O                           |
| 68  | P51                           | General-purpose I/O                           |
| 69  | P52/SO31                      | Serial output 31                              |
| 70  | P53/SI31/SDA31                | Serial input 31 / I2C data 31                 |
| 71  | P54/SCK31/SCL31               | Serial clock 31 / I2C clock 31                |
| 72  | P55/(PCLBUZ1)/(SCK00)         | Buzzer 1 / serial clock 00 (PIOR)             |
| 73  | P56/(INTP1)                   | External interrupt 1 (PIOR)                   |
| 74  | P57/(INTP3)                   | External interrupt 3 (PIOR)                   |
| 75  | P17/TI02/TO02/(SO00)/(TxD0)   | Timer I/O 2 / serial out 00 / UART Tx 0 (PIOR)|
| 76  | P16/TI01/TO01/INTP5/(SI00)/(RxD0) | Timer I/O 1 / ext. int. 5 / serial in 00 / UART Rx 0 (PIOR) |
| 77  | P15/SCK20/SCL20/(TI02)/(TO02) | Serial clock 20 / I2C clock 20 / Timer I/O 2 (PIOR) |
| 78  | P14/RxD2/SI20/SDA20/(SCLA0)/(TI03)/(TO03) | UART Rx 2 / serial in 20 / I2C data 20 / I2C clock 0 (PIOR) |
| 79  | P13/TxD2/SO20/(SDAA0)/(TI04)/(TO04) | UART Tx 2 / serial out 20 / I2C data 0 (PIOR) |
| 80  | P12/SO00/TxD0/TOOLTxD/(INTP5)/(TI05)/(TO05) | Serial out 00 / UART Tx 0 / debug data out (PIOR) |
| 81  | P11/SI00/RxD0/TOOLRxD/SDA00/(TI06)/(TO06) | Serial in 00 / UART Rx 0 / debug data in / I2C data 00 (PIOR) |
| 82  | P10/SCK00/SCL00/(TI07)/(TO07) | Serial clock 00 / I2C clock 00 / Timer I/O 7 (PIOR) |
| 83  | P90                           | General-purpose I/O                           |
| 84  | P91                           | General-purpose I/O                           |
| 85  | P92                           | General-purpose I/O                           |
| 86  | P93                           | General-purpose I/O                           |
| 87  | P94                           | General-purpose I/O                           |
| 88  | P95/SCK11/SCL11               | Serial clock 11 / I2C clock 11                |
| 89  | P96/SI11/SDA11                | Serial input 11 / I2C data 11                 |
| 90  | P97/SO11                      | Serial output 11                              |
| 91  | P112                          | General-purpose I/O                           |
| 92  | P113                          | General-purpose I/O                           |
| 93  | P114                          | General-purpose I/O                           |
| 94  | P115/ANI26                    | ADC input 26                                  |
| 95  | P116/ANI25                    | ADC input 25                                  |
| 96  | P117/ANI24                    | ADC input 24                                  |
| 97  | P101                          | General-purpose I/O                           |
| 98  | P110/(INTP10)                 | External interrupt 10 (PIOR)                  |
| 99  | P111/(INTP11)                 | External interrupt 11 (PIOR)                  |
| 100 | P146/(INTP4)                  | External interrupt 4 (PIOR)                   |
| 101 | P147/ANI18                    | ADC input 18                                  |
| 102 | P156/ANI14                    | ADC input 14                                  |

### Left Edge (pins 103–128, top to bottom)

| Pin | Signal                        | Alternate Functions                           |
|-----|-------------------------------|-----------------------------------------------|
| 103 | P155/ANI13                    | ADC input 13                                  |
| 104 | P154/ANI12                    | ADC input 12                                  |
| 105 | P153/ANI11                    | ADC input 11                                  |
| 106 | P152/ANI10                    | ADC input 10                                  |
| 107 | P151/ANI9                     | ADC input 9                                   |
| 108 | P150/ANI8                     | ADC input 8                                   |
| 109 | P27/ANI7                      | ADC input 7                                   |
| 110 | P26/ANI6                      | ADC input 6                                   |
| 111 | P25/ANI5                      | ADC input 5                                   |
| 112 | P24/ANI4                      | ADC input 4                                   |
| 113 | P23/ANI3                      | ADC input 3                                   |
| 114 | P22/ANI2                      | ADC input 2                                   |
| 115 | P21/ANI1/AVREFM               | ADC input 1 / ADC reference (-)               |
| 116 | P20/ANI0/AVREFP               | ADC input 0 / ADC reference (+)               |
| 117 | P130                          | General-purpose I/O                           |
| 118 | P102/TI06/TO06                | Timer I/O 6                                   |
| 119 | P07                           | General-purpose I/O                           |
| 120 | P04/SCK10/SCL10               | Serial clock 10 / I2C clock 10                |
| 121 | P03/ANI16/SI10/RxD1/SDA10     | ADC 16 / serial in 10 / UART Rx 1 / I2C data 10 |
| 122 | P02/ANI17/SO10/TxD1           | ADC 17 / serial out 10 / UART Tx 1           |
| 123 | P01/TO00                      | Timer output 0                                |
| 124 | P00/TI00                      | Timer input 0                                 |
| 125 | P145/TI07/TO07                | Timer I/O 7                                   |
| 126 | P144/SO30/TxD3                | Serial out 30 / UART Tx 3                     |
| 127 | P143/SI30/RxD3/SDA30          | Serial in 30 / UART Rx 3 / I2C data 30        |
| 128 | P142/SCK30/SCL30              | Serial clock 30 / I2C clock 30                |

---

## Port Summary

| Port   | Pins              | Count |
|--------|-------------------|-------|
| Port 0 | P00–P07           | 8     |
| Port 1 | P10–P17           | 8     |
| Port 2 | P20–P27           | 8     |
| Port 3 | P30–P37           | 8     |
| Port 4 | P40–P47           | 8     |
| Port 5 | P50–P57           | 8     |
| Port 6 | P60–P67           | 8     |
| Port 7 | P70–P77           | 8     |
| Port 8 | P80–P87           | 8     |
| Port 9 | P90–P97           | 8     |
| Port 10| P100–P106         | 7     |
| Port 11| P110–P117         | 8     |
| Port 12| P120–P127         | 8     |
| Port 13| P130, P137        | 2     |
| Port 14| P140–P147         | 8     |
| Port 15| P150–P156         | 7     |
| **Total GPIO** |            | **120** |

**Dedicated system pins (8):** VDD (×2), VSS (×2), EVDD0, EVSS0, EVDD1, EVSS1, REGC, RESET

**Shared system pins (type 1-2 / 2-1 / 2-2):** X1/P121, X2/P122, XT1/P123, XT2/P124, P40/TOOL0, P137/INTP0

> Pin Type refers to I/O circuit type per R01UH0146 §2.1 Pin Functions.

---

## References

- **Board User's Manual:** QB-R5F100SL-TB User's Manual Rev.2.00 (R20UT0827XJ)
- **Hardware Manual:** RL78/G13 User's Manual: Hardware R01UH0146 Rev.3.80
- **Datasheet:** RL78/G13 Datasheet (R01DS0131)
- **Package:** PLQP0128KD-A (128-pin LFQFP, 14×20 mm, 0.5 mm pitch)
- **Web:** https://www.renesas.com/en/design-resources/boards-kits/qb-r5f100sl-tb
