# STM32F407 Skystar Keil Environment Setup

This document helps a teammate reproduce the STM32F407 Skystar backup project
environment. It focuses on opening, compiling, downloading, and UART-checking
the existing Keil project in this repository.

Do not create a new CubeMX or HAL project for this path. The current Skystar
project uses the STM32F4 standard peripheral library.

## 1. Project Entry

Open this project in Keil uVision:

```text
firmware/stm32_f407/skystar_stdperiph_project/project/MDK(V5)/Project.uvprojx
```

The target board is:

```text
LCKFB Skystar STM32F407VET6
```

The Keil device should be:

```text
STM32F407VETx
```

## 2. Required Tools

Install these tools first:

| Tool | Purpose | Source |
| --- | --- | --- |
| Keil MDK-ARM / uVision | Open and build the project | https://www.keil.com/install/ |
| STM32F4xx device pack | Makes `STM32F407VETx` selectable in Keil | https://www.keil.arm.com/packs/stm32f4xx_dfp-keil/versions/ |
| ST-LINK USB driver | Lets Windows detect ST-Link/V2 or STLINK-V3 | https://www.st.com/en/development-tools/stsw-link009.html |
| USB serial driver | For CH340/USB-TTL debug output | Use the driver matching the adapter |

Known-good local setup used by the team:

```text
Keil MDK-ARM V5.39
ARM Compiler 6.21
Keil.STM32F4xx_DFP.2.17.1
ST-Link/V2 over SWD
```

Newer STM32F4xx DFP versions should also work, but if Keil cannot open the
existing `.uvprojx` cleanly, install `Keil.STM32F4xx_DFP.2.17.1` first because
that is the verified version.

## 3. Keil Pack Check

In Keil:

1. Open `Pack Installer`.
2. Search `STM32F407`.
3. Confirm `Keil::STM32F4xx_DFP` is installed.
4. Reopen the project.

If this error appears:

```text
Error: Device not found
Device: 'STM32F407VETx'
Vendor: 'STMicroelectronics'
Please update your device selection.
```

then the STM32F4 device pack is missing or not loaded. Install
`Keil::STM32F4xx_DFP`, then reopen the project.

## 4. Compiler Check

Open:

```text
Project -> Options for Target -> Target
```

Recommended:

```text
ARM Compiler: Use default compiler version 6
```

If the compiler path is missing, reinstall Keil MDK or add Arm Compiler through
the Keil installer. Do not switch this project to GCC or CubeIDE just to get a
first build.

## 5. Debugger and Flash Settings

Open:

```text
Project -> Options for Target -> Debug
```

Recommended settings:

```text
Use: ST-Link Debugger
Port: SW
Reset: Autodetect or SYSRESETREQ
```

Then open:

```text
Settings -> Flash Download
```

Recommended settings:

```text
Erase Sectors
Program: enabled
Verify: enabled
Reset and Run: optional
Programming Algorithm: STM32F4xx 512kB Flash
```

If the algorithm list is empty, add:

```text
STM32F4xx 512kB Flash
Address: 0x08000000 - 0x0807FFFF
```

## 6. Wiring for Download

Use ST-Link over SWD:

| ST-Link | Skystar STM32F407 |
| --- | --- |
| SWDIO | DIO |
| SWCLK | CLK |
| GND | GND |
| 3V3 / VTref | 3V3 reference |
| NRST | RST, optional |

Power the Skystar board normally from Type-C. The ST-Link 3V3 line is used as
target-voltage reference unless the adapter explicitly supports powering the
target.

## 7. Build and Download

In Keil:

1. Click `Rebuild`.
2. Expected result:

```text
0 Error(s)
```

3. Click `Download`.
4. If download succeeds, press the Skystar `RST` button.

Do not commit generated build outputs:

```text
Objects/
Listings/
*.hex
*.axf
*.map
*.o
```

## 8. UART Debug Check

Open a serial terminal at:

```text
115200 8N1
HEX display: off
```

Debug wiring:

| USB-TTL | Skystar |
| --- | --- |
| RXD | bottom `TX` / USART1_TX |
| TXD | bottom `RX` / USART1_RX, optional |
| GND | GND |
| VCC | not connected |

Expected boot output:

```text
SKYSTAR F407 VISION UART DEMO
DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, X42S: USART3 PB10/PB11, TJC reserve: USART6 PC6/PC7, KEY: PA0, OLED: PB8/PB9, 115200 8N1
```

If the output is garbled, check that the serial terminal is not set to
`230400`.

## 9. Fake MaixCAM Input Check

Fake MaixCAM input wiring:

| USB-TTL | Skystar |
| --- | --- |
| TXD | PA3 / USART2_RX |
| GND | GND |

Send this ASCII frame at `115200 8N1`:

```text
$V,1,320,240,80,80,150.0,8.0,0.0,0.99#
```

Expected output:

```text
VISION OK ...
VISION LINK CONNECTED
```

Stop sending frames for more than 500 ms. Expected output:

```text
VISION LINK TIMEOUT
```

## 10. Common Problems

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `Device not found STM32F407VETx` | STM32F4xx DFP missing | Install `Keil::STM32F4xx_DFP` |
| No `STM32F4xx 512kB Flash` algorithm | Device pack missing or target wrong | Install pack and select `STM32F407VETx` |
| Keil cannot see ST-Link | Driver missing or cable issue | Install `STSW-LINK009`; reconnect ST-Link |
| Build fails with compiler missing | Arm Compiler not installed | Modify Keil install and add Arm Compiler |
| UART output garbled | Wrong baud rate | Use `115200 8N1` |
| No UART output | RXD/TX/GND wiring issue | USB-TTL RXD to board TX, common GND |

## 11. What Not To Do

- Do not replace this project with a new CubeMX/HAL project.
- Do not change the board target away from `STM32F407VETx`.
- Do not commit Keil build outputs.
- Do not edit the MSPM0G3507 TianMengXing project while fixing the STM32
  environment.
- Do not use absolute paths from another teammate's computer in repository
  documents.

