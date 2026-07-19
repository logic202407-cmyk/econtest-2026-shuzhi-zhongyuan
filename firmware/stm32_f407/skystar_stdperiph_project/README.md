# Skystar STM32F407 StdPeriph Vision UART Demo

This project is based on the LCKFB Skystar STM32F407VET6 standard peripheral
library template.

Open this Keil project:

```text
project/MDK(V5)/Project.uvprojx
```

## UART Map

| Function | Peripheral | Pins | Baud |
| --- | --- | --- | --- |
| Debug printf | USART1 | PA9 TX / PA10 RX | 115200 |
| MaixCAM input | USART2 | PA2 TX / PA3 RX | 115200 |

Connection for the first UART test:

```text
MaixCAM TX  -> STM32 PA3 / USART2 RX
MaixCAM RX  <- STM32 PA2 / USART2 TX
MaixCAM GND -> STM32 GND
```

If only receiving fake vision data, PA2 can be left disconnected.

## Protocol

USART2 accepts both formats:

```text
$V,ver,seq,mode,cx,cy,w,h,distance,size,angle,valid#
$V,mode,cx,cy,w,h,D,x,angle,conf#
```

The second format is the early MaixCAM fake-data format.

Expected debug output on USART1:

```text
SKYSTAR F407 VISION UART DEMO
DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, 115200 8N1
VISION OK seq=1 mode=1 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LINK CONNECTED
```

If no valid frame is received for 500 ms, the LED is turned off and the debug
log prints `VISION LINK TIMEOUT`.

## Notes

- Keep this project on the standard peripheral library route. Do not mix HAL
  files into this project.
- USART1 is reserved for debug logs.
- USART2 is reserved for MaixCAM UART input in this demo.
- X42S RS485 code is not connected here yet; use this project first to verify
  the STM32F407 board, Keil environment, UART receive path, and MaixCAM fake
  data.
