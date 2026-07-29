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

Current camera resolution:

```python
FRAME_WIDTH = 640
FRAME_HEIGHT = 480
```

This is clearer than the early `320x240` bring-up setting and is preferred for
far-target gimbal tracking. If latency becomes more important than recognition
range, temporarily switch back to `320x240`.

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

The current version tracks a saturated red target by LAB threshold:

```python
COLOR_THRESHOLDS = [
    [10, 90, 35, 80, 0, 70],
]
```

If detection is unstable, tune this threshold with MaixVision or a threshold
editor. For first tests, use a large saturated red object under steady light.
The `A_min = 35` value is intentionally strict so that skin color is less likely
to be merged into the target box when the object is held by hand.

The current MaixCAM2 script also applies a lightweight target stabilizer:

| Item | Current behavior |
| --- | --- |
| Minimum blob gate | Accepts smaller far targets than the first bring-up script |
| Shape gate | Rejects very thin false blobs |
| Size/density gate | Rejects very large low-density boxes, such as hand plus target |
| Target selection | Prefers continuity with the previous target instead of always jumping to the largest blob |
| Output smoothing | Filters yaw/pitch at 20 Hz and limits one-frame jumps |
| Short dropout handling | Holds the last target for up to 2 missing frames, then sends `VISION,LOST` |

This should reduce static target jitter and avoid brief single-frame losses
without hiding a real target loss for long.

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

During a very short visual dropout, the script may continue sending the last
target for about 100 ms before reporting lost. This is intentional filtering.

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
