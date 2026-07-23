# X42S Motor Materials Summary

Original X42S manuals and examples are kept in the team material folder or
shared storage, not in Git. This page records the conclusions needed by the
project code.

## Protocol Choice

Use the X firmware free protocol first:

| Item | Current choice |
| --- | --- |
| Physical layer | RS485 |
| UART settings | 115200, 8N1 |
| Check byte | Fixed `0x6B` |
| Driver code | `application/motor/x42s_rs485/` |
| Modbus | Not used unless the motor checksum setting is changed to Modbus |

Do not send Modbus frames to a motor that is still using the default X firmware
free protocol.

## Current TianMengXing Wiring

| TianMengXing | RS485 module | X42S side |
| --- | --- | --- |
| B12 / UART3 TX | TXD / DI | RS485 A/B bus |
| B13 / UART3 RX | RXD / RO | RS485 A/B bus |
| GND | GND | Signal ground |
| 3V3 | VCC, if the module supports 3.3 V | - |
| B14 | DE and `/RE`, optional | Manual-direction modules only |

The current small TTL-RS485 module is automatic-direction, so `B14` is left
unconnected for that module.

## Safe Bring-Up Order

1. Confirm motor firmware type, address, baud rate, and power range on the
   motor screen.
2. Use USB-RS485 first to verify disable and read-position commands.
3. Connect TianMengXing UART3 B12/B13 to the RS485 module.
4. Test disable and read-position before any motion command.
5. Use low speed and small angle after the mechanical direction and limits are
   known.
6. Keep `GIMBAL_MOTION_ENABLED == 0U` until the gimbal safety checklist is
   complete.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| No reply | Motor address, baud rate, protocol, shared ground, and RS485 A/B polarity |
| Can send but cannot receive | Module RXD/RO to TianMengXing B13, module voltage level |
| Unexpected motion | Absolute/relative flag, direction bit, position unit, and mechanical zero |
