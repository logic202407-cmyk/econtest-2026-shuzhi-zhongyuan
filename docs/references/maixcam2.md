# MaixCAM2 Reference Summary

MaixCAM2 is the confirmed vision module for the main project direction.

## Official References

| Topic | URL |
| --- | --- |
| Hardware overview | https://wiki.sipeed.com/hardware/zh/maixcam/maixcam2.html |
| MaixPy UART | https://wiki.sipeed.com/maixpy/doc/zh/peripheral/uart.html |
| MaixPy pinmap | https://en.wiki.sipeed.com/maixpy/doc/en/peripheral/pinmap.html |
| Model deployment guide | https://wiki.sipeed.com/maixpy/doc/zh/ai_model_converter/ai_model_deploy.html |
| MaixCAM2 ONNX conversion | https://wiki.sipeed.com/maixpy/doc/zh/ai_model_converter/maixcam2.html |
| MaixPy source | https://github.com/sipeed/MaixPy |

## Project Wiring

| MaixCAM2 | TianMengXing MSPM0G3507 | Note |
| --- | --- | --- |
| A21 / UART4_TX | A9 / UART1_RX | Vision data to MSPM0 |
| A22 / UART4_RX | A8 / UART1_TX | Optional commands from MSPM0 |
| GND | GND | Required common ground |

Use `115200 8N1` for first bring-up.

Do not connect MaixCAM2 IO to 5 V logic. MaixCAM2 IO is 3.3 V.

## Software Entry

First bring-up script:

```text
firmware/maixcam2/main.py
```

It sends a 20 Hz binary `TARGET_FOUND` fake frame through `/dev/ttyS4` using
the project format in `docs/maixcam_protocol.md`. After UART wiring is
verified, replace `fake_target()` with the real vision algorithm output.

## Verified Bring-Up

Verified on 2026-07-24:

- MaixCAM2 `A21 / UART4_TX` outputs the known-good test frame
  `AA 55 01 06 7B 00 D3 FF 7A 26 F4`.
- TianMengXing `A9 / UART1_RX` receives and parses the frame, printing
  `VISION,FRAME` on the UART0 debug port.
- Keep `APP_VISION_RX_DEBUG_ENABLED == 0U` during normal tests. Per-byte UART0
  logging is slow enough to make UART1 drop bytes at the current polling setup.

## Model Notes

MaixCAM2 model packages normally use:

```text
.mud + .axmodel
```

Do not reuse MaixCAM / MaixCAM Pro `.cvimodel` files directly on MaixCAM2.
For custom target detection, prefer MaixHub training or an ONNX-to-MaixCAM2
conversion flow that produces `.mud` and `.axmodel` files.
