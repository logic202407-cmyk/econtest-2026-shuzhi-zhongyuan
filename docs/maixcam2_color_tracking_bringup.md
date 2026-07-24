# MaixCAM2 Color Tracking Bring-Up

This is the first real-vision test before enabling automatic gimbal motion.
It detects a colored target on MaixCAM2 and sends yaw/pitch error to
TianMengXing through the verified binary UART protocol.

## 1. Script

Run this script on MaixCAM2:

```text
firmware/maixcam2/main.py
```

Default mode:

```python
RUN_MODE = "color"
```

Fallback modes:

| Mode | Use |
| --- | --- |
| `"color"` | Real color-blob detection |
| `"fixed"` | Known-good frame `AA 55 01 06 7B 00 D3 FF 7A 26 F4` |
| `"fake"` | Small fake yaw sweep |

## 2. Wiring

| MaixCAM2 | TianMengXing | Notes |
| --- | --- | --- |
| `A21` / UART4_TX | `A9` / UART1_RX | Required |
| `A22` / UART4_RX | `A8` / UART1_TX | Optional |
| `GND` | `GND` | Required |
| `5V` / `3V3` | not connected | UART-only test does not need this |

Serial parameters:

```text
115200 8N1
```

## 3. Target Color

The first version tracks a bright red target by LAB threshold:

```python
COLOR_THRESHOLDS = [
    [20, 100, 20, 80, 0, 80],
]
```

If detection is unstable, tune this threshold with MaixVision or a threshold
editor. For first tests, use a large saturated red object under steady light.

## 4. Expected TianMengXing Output

Open the TianMengXing Type-C debug serial port at `115200 8N1`.

When a target is found:

```text
APP,INIT
VISION,TARGET,YAW,1.23,PITCH,-0.45,CONF,98.50
```

When no target is found:

```text
VISION,LOST
```

If only `VISION,TIMEOUT` appears, check MaixCAM2 wiring and confirm the script
is running. If output becomes unstable, keep `APP_VISION_RX_DEBUG_ENABLED`
disabled because per-byte debug printing can make UART1 drop bytes.

## 5. Temporary Direction Convention

The MaixCAM2 script currently sends:

| Target movement in image | Expected value |
| --- | --- |
| Target moves right | `yaw` increases |
| Target moves left | `yaw` decreases |
| Target moves up | `pitch` increases |
| Target moves down | `pitch` decreases |

Do not enable automatic gimbal motion until this sign convention is confirmed
with the real camera mount.

## 6. Safety

Keep the MSPM0G3507 project safety switch disabled during this test:

```c
GIMBAL_MOTION_ENABLED == 0U
```

This test should only prove:

```text
MaixCAM2 detects target -> TianMengXing receives yaw/pitch/confidence
```

It should not drive the X42S motors yet.
