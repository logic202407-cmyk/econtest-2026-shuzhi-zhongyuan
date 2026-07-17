# Serial Debug Tool

`serial_debug.py` generates the binary frames used by the MSPM0 gimbal chain.
Without `--port`, it only prints the frame in hexadecimal and requires only
Python 3. With `--port`, it sends at `115200 8N1` and requires `pyserial`:

```powershell
pip install pyserial
```

## MaixCAM UART1 Test Frames

Print a target-found frame with yaw/pitch in degrees and confidence in percent:

```powershell
python .\tools\serial_debug\serial_debug.py maixcam target `
  --yaw 1.23 --pitch -0.45 --confidence 98.5
```

Send the same frame every 50 ms to a USB-TTL adapter connected to MSPM0 UART1:

```powershell
python .\tools\serial_debug\serial_debug.py maixcam target `
  --yaw 1.23 --pitch -0.45 --confidence 98.5 `
  --port COM7 --repeat 100 --interval 0.05
```

Other frames:

```powershell
python .\tools\serial_debug\serial_debug.py maixcam lost
python .\tools\serial_debug\serial_debug.py maixcam heartbeat --uptime 1234 --sequence 1
python .\tools\serial_debug\serial_debug.py maixcam error --code 0x01
```

## X42S RS485 Frames

Use a USB-RS485 adapter and set the target motor to X firmware free protocol.
The tool uses the project's fixed `0x6B` check byte.

```powershell
python .\tools\serial_debug\serial_debug.py x42s enable --id 1 --port COM8
python .\tools\serial_debug\serial_debug.py x42s position `
  --id 1 --position 50 --speed 300 --acc 100 --dec 100 --port COM8
python .\tools\serial_debug\serial_debug.py x42s stop --id 1 --port COM8
python .\tools\serial_debug\serial_debug.py x42s read-position --id 1 --port COM8
```

`--position` is in `0.1 degree` units and `--speed` is in `0.1 RPM` units.
Start with a single motor, a small position such as `50` (5 degrees), and a
low speed. Confirm the motor ID and physical limits before sending commands.
