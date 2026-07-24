# MaixCAM2 Migration Plan

This document records the confirmed project switch from MaixCAM Pro to
MaixCAM2. The TianMengXing MSPM0G3507 side keeps the frozen UART1 assignment;
the main change is the MaixCAM-side hardware pins, MaixPy UART device, and
model format.

## 1. Migration conclusion

MaixCAM2 is now the preferred vision module for the main contest direction.
The host-side chain remains:

```text
MaixCAM2 -> UART1 -> TianMengXing MSPM0G3507 -> gimbal / car control
```

The MSPM0G3507 side does not need a pin change. The main change is on the
MaixCAM side: MaixCAM2 should use its own UART4 pins and MaixPy UART device.

## 2. Wiring

Recommended MaixCAM2 UART:

| MaixCAM2 pin | Function |
| --- | --- |
| A21 | UART4_TX |
| A22 | UART4_RX |
| GND | Common ground |

Connect to TianMengXing:

| MaixCAM2 | TianMengXing MSPM0G3507 | Note |
| --- | --- | --- |
| A21 / UART4_TX | A9 / UART1_RX | Vision data to MSPM0 |
| A22 / UART4_RX | A8 / UART1_TX | Optional command channel |
| GND | GND | Required |

TianMengXing keeps the frozen vision UART assignment:

```text
UART1: A8 TX / A9 RX
```

Do not confuse MaixCAM2 `A21/A22` with TianMengXing `A21/A22`; they are labels
on different boards.

## 3. MaixPy UART change

MaixCAM Pro commonly uses:

```text
A19 TX / A18 RX
/dev/ttyS1
```

MaixCAM2 should use:

```text
A21 TX / A22 RX
/dev/ttyS4
115200 8N1
```

Minimal MaixCAM2 UART initialization:

```python
from maix import uart, pinmap, err

err.check_raise(pinmap.set_pin_function("A21", "UART4_TX"), "set A21 UART4_TX failed")
err.check_raise(pinmap.set_pin_function("A22", "UART4_RX"), "set A22 UART4_RX failed")

serial = uart.UART("/dev/ttyS4", 115200)
serial.write(bytes((0xAA, 0x55, 0x01, 0x06, 0x7B, 0x00, 0xD3, 0xFF, 0x7A, 0x26, 0xF4)))
```

The repository now contains a MaixCAM2 UART bring-up script:

```text
firmware/maixcam2/main.py
```

It sends the formal binary `AA 55 ...` target frame used by the TianMengXing
MSPM0 application. The legacy ASCII `$V,...#` frame is kept only for older
USB-TTL and STM32 compatibility testing.

## 4. Protocol impact

No protocol change is required if MaixCAM2 sends the same bytes as MaixCAM Pro.
The MSPM0 parser should continue to treat the vision module as a UART byte
stream.

Current supported formats:

| Purpose | Document |
| --- | --- |
| Formal MSPM0 gimbal protocol | `docs/maixcam_protocol.md` |
| Legacy ASCII test protocol | `docs/uart_protocol.md` |

## 5. AI model impact

The main development difference is model deployment:

| Item | MaixCAM Pro | MaixCAM2 |
| --- | --- | --- |
| Recommended UART | A19/A18, `/dev/ttyS1` | A21/A22, `/dev/ttyS4` |
| Typical model artifact | `.cvimodel` | `.axmodel` |
| Host protocol | Same project protocol | Same project protocol |

Do not mix model files between the two platforms. For MaixCAM2, deploy model
packages as `.mud` plus `.axmodel`. The `.mud` file is loaded by MaixPy and
points to the actual `.axmodel` file or files.

## 6. Bring-up checklist

Before running real vision control:

1. Confirm the purchased board exposes A21/A22 as shown on the MaixCAM2 pin map.
2. Confirm MaixPy can list `/dev/ttyS4`.
3. Run a MaixCAM2 UART send-only test to a USB-TTL adapter.
4. Connect MaixCAM2 A21/A22 to TianMengXing A9/A8 and verify MSPM0 receives a
   known test frame.
5. Replace `fake_target()` in `firmware/maixcam2/main.py` with the real vision
   result.
6. Keep `GIMBAL_MOTION_ENABLED == 0U` until direction, zero, and limits are
   validated with vision and IMU data.

## 7. Keep current MSPM0 resources unchanged

The TianMengXing expansion allocation remains:

| Function | TianMengXing resource |
| --- | --- |
| Vision UART | A8 TX / A9 RX |
| X42S RS485 UART | B12 TX / B13 RX |
| RS485 direction | B14 |
| OLED | B4 SCL / B5 SDA |
| Car dual drive | B10/B11 and B15/B16 |
| Seven grayscale inputs | A21 to A27 |

The MaixCAM2 migration must not steal the TianMengXing grayscale pins; the
MaixCAM2 `A21/A22` pins are on the camera board only.

## 8. Official references

Use official Sipeed/MaixPy documents first when updating this area:

| Topic | Reference |
| --- | --- |
| MaixCAM2 hardware entry | https://wiki.sipeed.com/hardware/zh/maixcam/maixcam2.html |
| MaixPy UART | https://wiki.sipeed.com/maixpy/doc/zh/peripheral/uart.html |
| MaixPy pinmap | https://en.wiki.sipeed.com/maixpy/doc/en/peripheral/pinmap.html |
| MaixCAM2 model conversion | https://wiki.sipeed.com/maixpy/doc/zh/ai_model_converter/maixcam2.html |
| Model deployment guide | https://wiki.sipeed.com/maixpy/doc/zh/ai_model_converter/ai_model_deploy.html |

## 9. Current verification state

Confirmed by documentation:

- MaixCAM2 UART4 uses `A21 TX / A22 RX` and `/dev/ttyS4`.
- `115200 8N1` is the recommended first baud rate.
- MaixCAM2 IO is `3.3V`; do not connect 5V logic directly.
- MaixCAM2 model packages use `.mud` plus `.axmodel`.

Confirmed on real hardware, 2026-07-24:

- `firmware/maixcam2/main.py` runs on MaixCAM2 and sends the known-good binary
  frame `AA 55 01 06 7B 00 D3 FF 7A 26 F4`.
- USB-TTL capture on MaixCAM2 `A21 / UART4_TX` confirmed the exact bytes above.
- TianMengXing receives MaixCAM2 data through `A9 / UART1_RX` and prints
  `VISION,FRAME`, confirming the binary protocol parser accepts the frame.
- Per-byte UART0 logging can make UART1 drop bytes. Keep
  `APP_VISION_RX_DEBUG_ENABLED == 0U` during normal MaixCAM2 communication
  tests; use short diagnostic sessions only when raw byte tracing is required.

Still pending:

- Replace fake target data with the final vision algorithm output.
- Calibrate gimbal zero and fine direction using vision and IMU data.
