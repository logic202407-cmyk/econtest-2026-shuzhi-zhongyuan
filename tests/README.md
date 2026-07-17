# Application Logic Tests

`test_application_logic.c` is a host-side test for the hardware-independent
application logic. It verifies:

- MaixCAM target, lost-target, heartbeat, error, checksum, and length handling.
- Parser recovery after a corrupted frame.
- X42S free-protocol command bytes, fixed `0x6B` check byte, and status reply parsing.
- Gimbal target-loss stop behavior and yaw/pitch limit clamping.

Run it from the repository root on a Windows machine with either Visual Studio
Build Tools (`cl.exe`) or GCC (`gcc.exe`) available:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1
```

The script writes only to the ignored `build/host-tests/` directory. These are
not hardware tests and do not replace the later UART and RS485 board tests.
