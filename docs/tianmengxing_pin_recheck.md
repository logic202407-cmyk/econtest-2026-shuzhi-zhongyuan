# TianMengXing Pin Recheck

This note records the July pin recheck after reading the shared peripheral
materials.

The authoritative current allocation is:

```text
docs/tianmengxing_expansion_pin_map.md
application/config/app_config.h
```

## What Changed

The earlier allocation used:

```text
MaixCAM: UART1 B4/B5
X42S:    UART3 B12/B13, B14 DE
```

After considering the backup car peripherals, MaixCAM is moved back to
`UART1 A8/A9` because `A8` has already been TX-tested on the real board and
`B4/B5` are better used as the software-IIC OLED interface.

X42S remains on `UART3 B12/B13` with optional `B14` direction control. The
older `UART2 B15/B16` X42S plan is retired. After importing the
teammate-verified car project, `B15/B16` are reserved for the TJC serial screen.

## Current Frozen Mapping

| Function | Peripheral | Board pins | Notes |
| --- | --- | --- | --- |
| Debug log | UART0 | A10 TX / A11 RX | Board Type-C CH340E |
| MaixCAM | UART1 | A8 TX / A9 RX | A9 RX still needs final real-board receive verification |
| TJC 7-inch screen | UART2 | B15 TX / B16 RX | `TJC8048X270_011N`, tested at 115200 |
| X42S RS485 | UART3 | B12 TX / B13 RX | RS485 TTL side |
| RS485 direction | GPIO | B14 | Optional; unused on automatic-direction module |
| OLED | Soft IIC | B4 SCL / B5 SDA | 0.91 inch SSD1306 |
| AT8236 A motor | PWM | A12 / A13 | Teammate-verified backup car |
| AT8236 B motor | PWM | A26 / A27 | Teammate-verified backup car |
| Seven-channel grayscale | GPIO input | B25/B24/B20/B18/B19/B10/A7 | Teammate-verified backup car |
| Wheel encoders | GPIO input | A14/A15 and A24/A25 | Teammate-verified backup car |
| User LED | GPIO | B22 | Board LED |
| User button | GPIO | B21 | Active low |

## Resources To Avoid

| Board pins | Reason |
| --- | --- |
| A10/A11 | Debug UART |
| A19/A20 | SWD |
| B6/B7/B8/B9 | Onboard SPI Flash area |
| B21/B22 | User button and LED |
