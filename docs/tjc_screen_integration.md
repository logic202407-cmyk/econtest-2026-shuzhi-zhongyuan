# TJC 7-Inch Serial Screen Integration

This document records the tested TJC screen setup and the reserved interface for
later MSPM0G3507 and STM32F407 integration.

## 1. Tested Screen

The screen responded to the `connect` command at 115200 baud.

Returned model string:

```text
TJC8048X270_011N
```

Project assumptions:

| Item | Value |
| --- | --- |
| Screen model | `TJC8048X270_011N` |
| Resolution class | 800 x 480 |
| UART baud rate | 115200 |
| UART format | 8N1 |
| Command terminator | `FF FF FF` |

Validated commands:

| Command | HEX bytes | Result |
| --- | --- | --- |
| `connect` | `63 6F 6E 6E 65 63 74 FF FF FF` | Returns screen info |
| `bkcmd=3` | `62 6B 63 6D 64 3D 33 FF FF FF` | Returns `01 FF FF FF` |
| `dim=30` | ASCII plus `FF FF FF` | Brightness changes |
| `dim=100` | ASCII plus `FF FF FF` | Brightness restores |

## 2. Power

The 7-inch screen should use an external 5 V supply. Do not power it from the
small 3.3 V output of a USB-TTL module.

Minimum wiring for PC test:

| Power / USB-TTL | TJC screen |
| --- | --- |
| 5 V supply positive | 5V / VCC |
| 5 V supply negative | GND |
| USB-TTL GND | GND |
| USB-TTL TXD | RX |
| USB-TTL RXD | TX |

The 5 V supply and USB-TTL must share ground.

## 3. TianMengXing Reserved Interface

TianMengXing MSPM0G3507 reserves UART2 for the TJC screen:

| TianMengXing | TJC screen |
| --- | --- |
| B15 / UART2_TX | RX |
| B16 / UART2_RX | TX |
| GND | GND |

The application layer uses `application/display/tjc_screen/`.

`TJC_SCREEN_ENABLED` is currently `0U` in `application/config/app_config.h`.
Enable it only after the UART2 wiring is ready.

## 4. Suggested HMI Objects

For the first competition UI, use these object names so firmware code can update
the screen without guessing:

| Object | Purpose |
| --- | --- |
| `t_title` | Project title |
| `t_mode` | Current mode |
| `t_vis` | Vision status |
| `t_yaw` | Yaw value |
| `t_pitch` | Pitch value |
| `t_motor` | Motor/gimbal status |
| `bt_start` | Start button |
| `bt_stop` | Stop button |

Example firmware commands:

```text
t_mode.txt="GIMBAL"
t_vis.txt="OK"
t_yaw.txt="yaw=1.25"
t_pitch.txt="pitch=-0.80"
t_motor.txt="DISABLED"
```

Each command must end with:

```text
FF FF FF
```

## 5. STM32F407 Backup Integration

For the Skystar STM32F407 backup project, the TJC screen should be tested as a
separate UART peripheral before it is combined with MaixCAM or X42S code.
Keep the same protocol: ASCII command plus `FF FF FF`, 115200 8N1.

Reserved Skystar wiring:

| Skystar | TJC screen |
| --- | --- |
| PC6 / USART6_TX | RX |
| PC7 / USART6_RX | TX |
| GND | GND |
| independent 5V supply | 5V |

The Skystar backup driver is in
`firmware/stm32_f407/skystar_stdperiph_project/bsp/tjc/`.
`SKYSTAR_TJC_ENABLED` is currently `0U`; set it to `1U` only when testing the
screen on hardware.
