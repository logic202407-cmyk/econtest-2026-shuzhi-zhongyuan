# TianMengXing Expansion Pin Map

> **H题小车接线请优先使用 [H题当前引脚分配](h_problem_current_pin_map.md)。**
> 本文件保留了早期云台和旧扩展板方案，其中的 `PA26/PA25` 右轮分配、
> `PB15/PB16` 串口屏分配均不是当前 H 题有效接线。

This document freezes the current TianMengXing MSPM0G3507 pin allocation for
the vision/gimbal main path and the car backup path.

Use board labels when wiring: `A8`, `B12`, etc. The MSPM0 port-prefix form
is shown only when it helps match driver code.

## 1. Design Rules

- Keep `A10/A11` for the board Type-C debug UART.
- Keep `A19/A20` for SWD.
- Keep `B21` for the user button and `B22` for the user LED.
- Avoid `B6/B7/B8/B9`; they are used by the onboard SPI Flash area.
- Keep the gimbal and car resources independent so the backup car path does
  not steal the camera or X42S UART.
- Application code must include `application/config/app_config.h`; do not
  scatter raw pin names in business logic.

## 2. Main Vision/Gimbal Path

| Function | Peripheral | Board pins | Notes |
| --- | --- | --- | --- |
| Debug log | UART0 | A10 TX / A11 RX | Board Type-C CH340E, 115200 8N1 |
| MaixCAM2 | UART1 | A8 TX / A9 RX | A8 has been TX-tested on the real board; A9 RX still needs loopback/USB-TTL verification |
| X42S RS485 | UART3 | B12 TX / B13 RX | Replaces the older B15/B16 UART2 plan; these pins are adjacent and easier for the extension board |
| RS485 direction | GPIO | B14 | Optional; leave unconnected for automatic-direction RS485 modules |
| TJC 7-inch screen | UART2 | B15 TX / B16 RX | Reserved for the tested `TJC8048X270_011N`, 115200 8N1 |
| User LED | GPIO | B22 | Board LED |
| User button | GPIO | B21 | Active low |

RS485 wiring for the common automatic-direction module:

```text
TianMengXing B12  -> RS485 module TXD / DI
TianMengXing B13  <- RS485 module RXD / RO
TianMengXing 3V3  -> RS485 module VCC
TianMengXing GND  -> RS485 module GND
```

If a manual-direction RS485 module is used later, short `DE` and `/RE`
together and connect them to `B14`.

## 3. Display

| Function | Interface | Board pins | Notes |
| --- | --- | --- | --- |
| 0.91 inch SSD1306 OLED | software IIC | B4 SCL / B5 SDA | Uses ordinary GPIO; does not consume a UART |
| TJC 7-inch serial HMI | UART2 | B15 TX / B16 RX | Tested at 115200 through USB-TTL; command terminator is `FF FF FF` |
| SeekFree IPS2.0 PRO | SPI0 | B18 SCK / B17 MOSI / B19 MISO / B25 CS / A7 RST / A15 INT | E-problem bench display only; disabled for H-problem car bring-up |

The OLED is not part of the minimum gimbal motion loop. It is kept as a
debug/status display interface.

The IPS2.0 PRO allocation overlaps the H-problem car grayscale/current-sense
reserve pins. For the H-problem car build, keep `APP_IPS200PRO_SCREEN_ENABLED`
disabled and wire the grayscale sensor according to the car table below.

## 4. Backup Car Path

The AT8236 dual DC motor driver needs four logic/PWM inputs. The RYDZ
seven-channel grayscale sensor provides seven digital outputs and should be
read as pull-up GPIO inputs.

The mapping below follows the teammate-verified car project first. It replaces
the earlier speculative `B10/B11/B15/B16` motor plan and `A21-A27` grayscale
plan.

### AT8236 Dual Motor Driver

| Function | Board pin | Driver macro |
| --- | --- | --- |
| A motor IN1 | A12 | `CAR_MOTOR_A_IN1_PWM_PIN` |
| A motor IN2 | A13 | `CAR_MOTOR_A_IN2_PWM_PIN` |
| B motor IN1 | A26 | `CAR_MOTOR_B_IN1_PWM_PIN` |
| B motor IN2 | A27 | `CAR_MOTOR_B_IN2_PWM_PIN` |

The motor driver VM pin is a separate 5-12 V motor supply according to the
AT8236 module guide. Do not connect this VM rail to the X42S 24 V rail unless
the car motor and driver are explicitly rated for that voltage.

### Seven-Channel Grayscale Sensor

| Sensor output | Board pin | Driver macro |
| --- | --- | --- |
| OUT1 | B25 | `CAR_GRAY_OUT1_PIN` |
| OUT2 | B24 | `CAR_GRAY_OUT2_PIN` |
| OUT3 | B20 | `CAR_GRAY_OUT3_PIN` |
| OUT4 | B18 | `CAR_GRAY_OUT4_PIN` |
| OUT5 | B19 | `CAR_GRAY_OUT5_PIN` |
| OUT6 | B10 | `CAR_GRAY_OUT6_PIN` |
| OUT7 | A7 | `CAR_GRAY_OUT7_PIN` |

### Wheel Encoder Reserve

The teammate project also includes quadrature encoder inputs:

| Encoder signal | Board pin |
| --- | --- |
| A wheel phase A | A14 |
| A wheel phase B | A15 |
| B wheel phase A | A24 |
| B wheel phase B | A25 |

### Yaw Gyro

The teammate project uses an XV7001/XV7011-compatible single-axis gyro. Its
original A10/A11 software-IIC wiring conflicts with the TianMengXing Type-C
debug UART and is therefore not imported. The integrated application keeps a
separate software-IIC bus:

| Signal | Board pin | Notes |
| --- | --- | --- |
| SCL | A31 | Dedicated gyro software-IIC clock |
| SDA | A28 | Dedicated gyro software-IIC data |
| VCC | 3.3V | Do not use 5V unless the gyro board explicitly supports it |
| GND | GND | Common ground required |

The driver samples every 5 ms and performs a non-blocking 400-sample bias
calibration while the car remains stopped. A missing gyro changes its status to
failed; it does not block the main loop or enable the motors.

## 5. Reserved Expansion

| Purpose | Board pins | Driver macro |
| --- | --- | --- |
| Current sense 0 | B17 ADC | `APP_CURRENT_SENSE0_ADC_PIN` |
| Current sense 1 | not assigned | `APP_CURRENT_SENSE1_ADC_PIN` |
| Auxiliary PWM 0 | B2 | `APP_AUX_PWM0_PIN` |
| Auxiliary PWM 1 | B3 | `APP_AUX_PWM1_PIN` |

`B18` is now owned by grayscale OUT4, so the second current-sense channel is
intentionally left unassigned until the power/current detection design is
confirmed. Avoid using `B17` as miscellaneous GPIO before that decision.

## 6. Retired Historical Allocation

The older documents used:

```text
X42S RS485: UART2 B15 TX / B16 RX, B17 DE
```

That allocation is retired because X42S now uses `UART3 B12/B13` and optional
`B14` direction control. After importing the teammate-verified car project,
`B15/B16` are reserved for the TJC serial screen instead of the AT8236 driver.
