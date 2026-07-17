# Debug Logging

UART0 (`PA10` TX, `PA11` RX, on-board CH340E) is the only application debug
output. It runs at `115200 8N1` and is never shared with MaixCAM or X42S.

Logging is controlled by `APP_DEBUG_LOG_ENABLED` in `application/config/app_config.h`.
The current messages are intentionally event-based:

| Message | Meaning |
| --- | --- |
| `APP,INIT` | Application, platform, UART, vision parser, and gimbal initialization completed. |
| `VISION,FRAME` | A complete, valid MaixCAM binary frame was accepted. |
| `VISION,TIMEOUT` | A previously active vision stream exceeded the 500 ms timeout; the gimbal stop path has run. |

The implementation sends through the SeekFree debug UART buffer from the
platform layer. Do not add per-byte or per-control-cycle logs: UART0 output is
for state transitions and fault localization, not telemetry streaming.
