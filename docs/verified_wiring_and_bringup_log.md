# Verified Wiring and Bring-Up Log

This document records wiring and test results that have already worked on real
hardware. Read this before changing pin maps, UART ports, or gimbal commands.

## 1. Current Verified Hardware

| Module | Current status |
| --- | --- |
| Main controller | TianMengXing MSPM0G3507 |
| Vision module | MaixCAM2 |
| Gimbal motors | Two ZDT X42S closed-loop stepper motors |
| Motor protocol | Emm firmware free protocol, `115200 8N1`, fixed check byte `0x6B` |
| RS485 module | Automatic-direction TTL-RS485 module |
| Debug port | TianMengXing Type-C CH340E, UART0 |

Current gimbal motor mapping:

| Motor | ID | Physical position | Function |
| --- | --- | --- | --- |
| Pitch | `1` | Upper motor | Camera pitch / up-down |
| Yaw | `2` | Lower motor | Horizontal rotation / left-right |

## 2. TianMengXing Pin Map Used in Tests

Use board labels such as `A8` and `B12` when wiring. Do not describe these to
teammates only as MCU package pins.

| Function | TianMengXing label | Notes |
| --- | --- | --- |
| UART0 debug TX/RX | `A10` / `A11` | Board Type-C CH340E |
| MaixCAM2 UART1 TX/RX | `A8` / `A9` | TianMengXing UART1 |
| X42S UART3 TX/RX | `B12` / `B13` | TianMengXing UART3 |
| RS485 direction reserve | `B14` | Not connected with current automatic-direction module |
| User key | `B21` | Low-level active |
| User LED | `B22` | Board LED |

## 3. MaixCAM2 to TianMengXing Wiring

Verified wiring:

| MaixCAM2 | TianMengXing | Required |
| --- | --- | --- |
| `A21` / UART4_TX | `A9` / UART1_RX | Yes |
| `A22` / UART4_RX | `A8` / UART1_TX | Optional for current one-way vision test |
| `GND` | `GND` | Yes |
| `5V` / `3V3` | not connected | Do not connect during UART-only test |

MaixCAM2 test script:

```text
firmware/maixcam2/main.py
```

The verified test version sends a known-good binary frame from MaixCAM2 UART4:

```text
AA 55 01 06 7B 00 D3 FF 7A 26 F4
```

Expected TianMengXing debug output on UART0:

```text
APP,INIT
VISION,FRAME
VISION,FRAME
...
```

Important debugging note:

- If per-byte debug is enabled on TianMengXing, UART0 printing can block the
  UART1 receive polling and only partial bytes such as `AA` and `F4` may appear.
- Keep `APP_VISION_RX_DEBUG_ENABLED == 0U` for normal MaixCAM2 link testing.

## 4. TianMengXing to X42S RS485 Wiring

The current TTL-RS485 module is automatic-direction. Its TTL-side silk screen
has been verified with the following non-crossed connection:

| TianMengXing | TTL-RS485 module | Notes |
| --- | --- | --- |
| `B12` / UART3_TX | `TXD` | Main controller TX |
| `B13` / UART3_RX | `RXD` | Main controller RX |
| `3.3V` | `VCC` | Use 3.3 V with the current module |
| `GND` | `GND` | Must share ground with motor power negative |

RS485 bus side:

| TTL-RS485 module | X42S transfer board |
| --- | --- |
| `A+` | `R/A/H` |
| `B-` | `T/B/L` |
| `GND` | common ground with transfer board and 24 V power negative |

The following grounds must be connected together:

```text
TianMengXing GND
TTL-RS485 module GND
X42S transfer board GND
24 V motor power negative
```

`B14` is reserved for a future manual-direction RS485 module. It is not wired
to the current automatic-direction module.

## 5. X42S Motor Power Wiring

The two X42S motors share the same 24 V motor power bus through the transfer
board.

| Power | Connection |
| --- | --- |
| 24 V adapter positive | Transfer board power input positive / motor `V+` |
| 24 V adapter negative | Transfer board power input negative / motor `Gnd` |

Using one 24 V adapter for two motors is normal as long as its current capacity
is enough. The current team adapter is `24 V 6 A`; keep motion slow during
early tests and watch motor temperature.

## 6. Verified X42S Behavior

The two motors have both been verified through USB-RS485 and TianMengXing.

Known working observations:

| Test | Result |
| --- | --- |
| ID1 read position | Returned X42S frame |
| ID2 read position | Returned X42S frame after restoring ID2 |
| ID1 enable | Shaft locked and restored after hand twist |
| ID2 enable | Shaft locked after restoring its ID to `2` |
| `+20 deg` relative command | Both motors reached about `+20 deg` |
| `-20 deg` relative command | Both motors reached about `-20 deg` |
| Stop command | Both motors can be stopped |

For the B21 manual test firmware, the useful sequence is:

| Key press count | Action |
| --- | --- |
| 1 | Enable/read ID1 and ID2 |
| 2 | Send both axes `+20 deg` relative move |
| 3 | Send both axes `-20 deg` relative move |
| 4 | Stop both axes |

The position command must use relative motion for this manual test. Do not
replace it with an absolute-position command unless the zero point has been
defined and tested.

## 7. Verified Direction

With the current mechanical build:

| Axis | Motor ID | `+20 deg` relative command result |
| --- | --- | --- |
| Pitch | `1` | Camera pitches down |
| Yaw | `2` | Gimbal turns left |

Fine direction calibration should wait until real vision and IMU data are
available. For now, keep the automatic gimbal range small.

## 8. Current Safety State

Keep this default until mechanical zero, soft limits, and camera direction are
confirmed:

```c
GIMBAL_MOTION_ENABLED == 0U
```

Recommended temporary software range:

```text
Yaw:   -20 deg to +20 deg
Pitch: -20 deg to +20 deg
```

## 9. Next Recommended Tests

1. Run MaixCAM2 real detection, but only print parsed yaw/pitch/confidence on
   TianMengXing first.
2. Confirm the sign of visual yaw and pitch by moving a target left/right and
   up/down.
3. Enable one-axis low-gain gimbal response only after the printed vision sign
   is understood.
4. Expand motion range only after cables, hard limits, and printed-frame
   direction are checked.

## 10. 2026-07-25 Yaw Tracking Stable Baseline

Commit `e056efc` is the current preferred yaw-axis tracking baseline.

Observed behavior:

| Case | Result |
| --- | --- |
| Target is static near center | Gimbal holds without continuous left-right oscillation |
| Target overshoots center | Gimbal may make one reverse correction, then stops |
| Target lost | Motor remains locked and no continued runaway motion is expected |
| Moving target | Usable for continued tuning, but response speed and far-target stability can still improve |

Key control choices in this baseline:

| Parameter | Value | Purpose |
| --- | --- | --- |
| Center deadband | `6 deg` | Avoid jitter around the crosshair |
| Restart deadband | `9 deg` | Avoid immediately reversing after a stop |
| Stop settle time | `180 ms` | Let the mechanical system settle before restarting |
| Yaw max speed | `12 rpm` | Keep motion smooth and safe |
| Yaw min speed | `0.8 rpm` | Allow small corrections without jerky starts |

Do not replace this baseline with a more aggressive setting unless the new
version is tested against static target hold, sudden target stop, target loss,
and far-target tracking.
