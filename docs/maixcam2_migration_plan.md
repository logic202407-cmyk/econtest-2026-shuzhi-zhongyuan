# MaixCAM2 Migration Plan

This document records the planned changes if the vision module is changed from
MaixCAM Pro to MaixCAM2. The current firmware still targets the existing
MaixCAM Pro link unless the team explicitly switches hardware.

## 1. Migration conclusion

MaixCAM2 can keep the same host-side protocol:

```text
vision module -> UART -> TianMengXing MSPM0G3507 -> gimbal / car control
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
serial.write_str("$V,1,320,240,80,80,150.0,8.0,0.0,0.99#")
```

For the formal binary protocol, keep the same frame definition in
`docs/maixcam_protocol.md`; only replace the UART initialization and the AI
model runtime.

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

Do not mix model files between the two platforms. If the team switches to
MaixCAM2, retrain or convert the model for MaixCAM2 and update the MaixPy
vision script accordingly.

## 6. Switch checklist

Before changing the project default from MaixCAM Pro to MaixCAM2:

1. Confirm the purchased board exposes A21/A22 as shown on the MaixCAM2 pin map.
2. Confirm MaixPy can list `/dev/ttyS4`.
3. Run a MaixCAM2 UART send-only test to a USB-TTL adapter.
4. Connect MaixCAM2 A21/A22 to TianMengXing A9/A8 and verify MSPM0 receives a
   known test frame.
5. Only then replace the MaixCAM Pro UART initialization in the project vision
   script.

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
