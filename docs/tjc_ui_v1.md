# TJC 7-Inch UI V1

This is the first operational UI for `TJC8048X270_011N` (800 x 480,
landscape). It is designed for the verified TianMengXing car test first, while
keeping the gimbal/vision status fields ready for the main system.

## 1. Safety Rule

The screen must never become the only stop mechanism. `bt_stop` is an
additional software stop request. The physical reset, power switch, and any
mechanical safety measures remain authoritative.

The current verified car demo starts automatically on power-up. Do not enable
screen start control until the new touch-controlled car loop is separately
compiled and tested on blocks. The V1 screen can be completed and burned now;
the start button will be a reserved request until that code is enabled.

## 2. Page 0: Run Page

Create page `p0` with a dark background and the following objects. Keep every
object name and component ID exactly as shown.

| Type | Name | Component ID | Display / action |
| --- | --- | ---: | --- |
| Text | `t_title` | 1 | `26 Dian Sai Smart System` |
| Text | `t_mode` | 2 | Mode: `CAR`, `GIMBAL`, or `IDLE` |
| Text | `t_run` | 3 | Run state: `READY`, `RUNNING`, or `STOPPED` |
| Text | `t_vis` | 4 | Vision link / target state |
| Text | `t_gray` | 5 | Seven-channel grayscale bits |
| Text | `t_speed` | 6 | Left/right wheel speed |
| Text | `t_bat` | 7 | Battery / current state, initially `--` |
| Text | `t_hint` | 8 | Short operator message |
| Button | `bt_start` | 11 | Green, label `START` |
| Button | `bt_stop` | 12 | Red, label `STOP` |
| Button | `bt_debug` | 13 | Label `DEBUG` and switch to `p1` |

Suggested layout:

```text
+------------------------------------------------------------------+
|  26 DIAN SAI SMART SYSTEM                MODE: CAR               |
|                                                                  |
|  RUN: READY                 VISION: --                           |
|                                                                  |
|  GRAY: 0000000             SPEED: L --  R --                    |
|                                                                  |
|  BATTERY: --               HINT: Touch START after ready        |
|                                                                  |
|        [ START ]             [ STOP ]             [ DEBUG ]     |
+------------------------------------------------------------------+
```

## 3. Page 1: Debug Page

Create page `p1` with the following objects.

| Type | Name | Component ID | Display / action |
| --- | --- | ---: | --- |
| Text | `t_dbg_title` | 1 | `DEBUG` |
| Text | `t_left` | 2 | Left motor target / actual speed |
| Text | `t_right` | 3 | Right motor target / actual speed |
| Text | `t_corr` | 4 | Line-follow correction |
| Text | `t_yaw` | 5 | Gimbal yaw (reserved) |
| Text | `t_pitch` | 6 | Gimbal pitch (reserved) |
| Text | `t_motor` | 7 | X42S motor state (reserved) |
| Button | `bt_back` | 11 | Return to `p0` |

Suggested layout:

```text
+------------------------------------------------------------------+
|  DEBUG                                                           |
|                                                                  |
|  LEFT: --                  RIGHT: --                             |
|  LINE CORRECTION: --       GRAY: 0000000                         |
|  YAW: --                   PITCH: --                             |
|  X42S: DISABLED                                                    |
|                                                                  |
|                         [ BACK ]                                 |
+------------------------------------------------------------------+
```

## 4. Touch Return Protocol

For every button, enable the TJC property that sends its component ID on touch
release. The screen then returns the standard frame:

```text
65 <page_id> <component_id> <event> FF FF FF
```

Use only release events (`event = 01`) for commands. Ignore press events
(`event = 00`) so a finger held on a button cannot generate repeated actions.

| Page | Component ID | Release action | Firmware meaning |
| ---: | ---: | --- | --- |
| 0 | 11 | `bt_start` | Request car start; ignored until touch-controlled car mode is enabled |
| 0 | 12 | `bt_stop` | Request immediate software stop |
| 0 | 13 | `bt_debug` | Switch to `p1` in HMI script |
| 1 | 11 | `bt_back` | Switch to `p0` in HMI script |

For `bt_debug` and `bt_back`, use a Touch Release Event script in the HMI
project:

```text
page p1
```

and:

```text
page p0
```

respectively.

## 5. Firmware Object Contract

The existing TJC driver already updates these text objects:

```text
t_mode
t_vis
t_yaw
t_pitch
t_motor
```

Future car-screen integration adds `t_run`, `t_gray`, `t_speed`, `t_bat`,
`t_hint`, `t_left`, `t_right`, and `t_corr`. Do not rename existing objects in
the HMI project; firmware sends object names as ASCII commands.

## 6. First Hardware Test

After the screen project is compiled and burned:

1. Power the screen from an independent stable 5 V supply.
2. Wire TianMengXing `B15` to screen RX, `B16` to screen TX, and connect GND.
3. Set `TJC_SCREEN_ENABLED` to `1U` only in the test branch/project.
4. Confirm `t_mode`, `t_vis`, `t_yaw`, `t_pitch`, and `t_motor` update.
5. Confirm button return frames on the debug UART before enabling vehicle
   motion from touch.
