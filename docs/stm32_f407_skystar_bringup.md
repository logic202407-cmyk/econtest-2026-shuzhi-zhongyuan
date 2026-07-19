# STM32F407 Skystar Bring-Up Notes

This note describes the backup STM32F407 Skystar project used to verify the
MaixCAM ASCII UART link before the MSPM0G3507 hardware is available.

## Project Entry

Open this Keil project:

```text
firmware/stm32_f407/skystar_stdperiph_project/project/MDK(V5)/Project.uvprojx
```

The project is based on the LCKFB Skystar STM32F407VET6 standard peripheral
library template. Keep this project on the StdPeriph path; do not mix CubeMX/HAL
files into it.

## UART Map

| Function | Peripheral | Pins | Baud |
| --- | --- | --- | --- |
| Debug printf | USART1 | PA9 TX / PA10 RX | 115200 8N1 |
| MaixCAM fake input | USART2 | PA2 TX / PA3 RX | 115200 8N1 |

## Key Map

The on-board user key follows the official LCKFB `006按键点灯` example:

| Function | Pin | Electrical state |
| --- | --- | --- |
| User KEY | PA0 | Pulldown input, pressed = high level |

When the key is pressed, the debug UART prints:

```text
KEY PRESS count=1
```

The OLED fourth line also shows the key count, so the key can be verified
without watching the serial terminal all the time.

## OLED Map

The backup project now supports the 0.91 inch 4-pin white I2C OLED module
with an SSD1306-compatible controller.

Default wiring:

```text
OLED VCC -> Skystar 3V3
OLED GND -> Skystar GND
OLED SCL -> Skystar PB8
OLED SDA -> Skystar PB9
```

The driver uses software I2C, so it does not require extra Keil peripheral
configuration. The default I2C write address is `0x78`, which corresponds to
the common 7-bit OLED address `0x3C`.

Expected OLED pages after reset:

```text
SKYSTAR F407
WAIT VISION
USART2 PA2 PA3
115200 8N1
```

After a valid or lost MaixCAM frame arrives, the OLED shows the current vision
state, `cx/cy`, angle, distance, sequence number, and mode. If the link times
out, it shows `VISION TIMEOUT`.

On the Skystar board bottom debug header, the debug UART is marked as `TX`,
`RX`, and `GND`.

USB-TTL wiring for debug output:

```text
USB-TTL RXD -> Skystar TX  / USART1_TX
USB-TTL TXD -> Skystar RX  / USART1_RX
USB-TTL GND -> Skystar GND
USB-TTL VCC -> not connected
```

For printf output only, `USB-TTL RXD -> Skystar TX` plus common GND is enough.
Power the Skystar board separately from its own Type-C connector.

USB-TTL wiring for fake MaixCAM input:

```text
USB-TTL TXD -> Skystar PA3 / USART2_RX
USB-TTL GND -> Skystar GND
```

If using one USB-TTL module for both directions during this demo:

```text
USB-TTL RXD -> Skystar TX  / USART1_TX
USB-TTL TXD -> Skystar PA3 / USART2_RX
USB-TTL GND -> Skystar GND
```

## Keil Setup

Verified local setup:

- Keil MDK-ARM V5.39
- ARM Compiler 6.21
- `Keil.STM32F4xx_DFP.2.17.1`
- ST-Link/V2 over SWD

The project is configured for ARM Compiler 6 and disables the old CMSIS FPU
path used by this template. This avoids the ARMCLANG `vfpcc` inline assembly
compatibility error. It also suppresses `invalid UTF-8` warnings caused by
legacy GBK comments in vendor files.

## Flashing

Keil debug settings:

- Debug adapter: `ST-Link Debugger`
- Port: `SW`
- Flash algorithm: `STM32F4xx 512kB Flash`
- Recommended download options: `Program`, `Verify`, and optionally
  `Reset and Run`

A successful rebuild/download should produce `0 Error(s)` and generate:

```text
project/MDK(V5)/Objects/Project.hex
```

Do not commit Keil build products such as `Objects/`, `Listings/`, `.hex`,
`.axf`, `.map`, or `.o`.

## Expected Boot Log

Open the USB-TTL COM port at `115200 8N1`, then press the Skystar reset button.
Expected output:

```text
SKYSTAR F407 VISION UART DEMO
DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, 115200 8N1
```

Newer firmware builds include the key and OLED map in the second line:

```text
DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, KEY: PA0, OLED: PB8/PB9, 115200 8N1
```

If the text is garbled, check the baud rate first. A common mistake is leaving
the serial tool at `230400`.

## Fake Vision Frames

Legacy MaixCAM-style ASCII frame:

```text
$V,1,320,240,80,80,150.0,8.0,0.0,0.99#
```

Expected debug output:

```text
VISION OK seq=1 mode=1 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LINK CONNECTED
```

Coordinate and angle test:

```text
$V,1,100,200,60,40,120.0,6.5,-12.3,0.88#
```

Expected output includes:

```text
cx=100 cy=200 w=60 h=40 dist=120.0cm size=6.5cm angle=-12.3deg
```

Target-lost test:

```text
$V,0,320,240,80,80,150.0,8.0,0.0,0.10#
```

Expected output:

```text
VISION LOST ...
```

Validity rule for this demo:

- V1 format: `mode != 0 && valid != 0`
- Legacy format: `mode != 0 && conf >= 0.50`

If no valid frame arrives for more than 500 ms, expected output:

```text
VISION LINK TIMEOUT
```

## Verified Status

Verified on board:

- ST-Link download works.
- USART1 debug output works through USB-TTL/CH340.
- USART2 receives fake MaixCAM ASCII frames through USB-TTL.
- Continuous 100 ms frames stay connected.
- Stopping frames triggers `VISION LINK TIMEOUT`.
- Different `cx/cy/w/h/distance/size/angle` values are parsed correctly.
- `mode=0` or low confidence is treated as target lost after the parser fix.

## 2026-07-19 Board Test Log

Hardware used:

- Skystar STM32F407VET6 board
- ST-Link/V2 for flashing
- USB-SERIAL CH340 USB-TTL for UART testing
- SSCOM at `115200 8N1`

Connections:

```text
USB-TTL RXD -> Skystar TX  / USART1_TX
USB-TTL TXD -> Skystar PA3 / USART2_RX
USB-TTL GND -> Skystar GND
```

Boot output after pressing reset:

```text
SKYSTAR F407 VISION UART DEMO
DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, 115200 8N1
```

Normal target frame:

```text
$V,1,100,200,60,40,120.0,6.5,-12.3,0.88#
```

Observed output:

```text
VISION OK seq=22 mode=1 cx=100 cy=200 w=60 h=40 dist=120.0cm size=6.5cm angle=-12.3deg
VISION LINK CONNECTED
VISION LINK TIMEOUT
```

Target-lost frame:

```text
$V,0,320,240,80,80,150.0,8.0,0.0,0.10#
```

Observed output after parser fix:

```text
VISION LOST seq=1 mode=0 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LOST seq=2 mode=0 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LOST seq=3 mode=0 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LOST seq=4 mode=0 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
```

Conclusion:

- The STM32F407 backup UART demo is board-verified.
- The `mode=0` lost-target rule works on hardware.
- The next STM32F407 backup tasks can move to OLED, key input, and PWM output.

## Next Steps

After this UART demo is stable, Kai can continue with:

- OLED display for `cx`, `cy`, and `angle`
- key input verification
- PWM output for servo or motor control

Keep this project as the STM32F407 backup path. The MSPM0G3507 main path remains
the priority once the Tianmengxing board and debugger arrive.
