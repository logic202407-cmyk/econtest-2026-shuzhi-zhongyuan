# Application Logic Tests

`test_application_logic.c` is a host-side test for the hardware-independent
application logic. It verifies:

- MaixCAM target, lost-target, heartbeat, error, checksum, and length handling.
- Parser recovery after a corrupted frame.
- X42S free-protocol command bytes, fixed `0x6B` check byte, and status reply parsing.
- Gimbal target-loss stop behavior and yaw/pitch limit clamping.
- Default gimbal motion lock: initialization disables both motors and valid
  vision frames do not generate position commands.

Run it from the repository root on a Windows machine with either Visual Studio
Build Tools (`cl.exe`) or GCC (`gcc.exe`) available:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1
```

The script writes only to the ignored `build/host-tests/` directory. These are
not hardware tests and do not replace the later UART and RS485 board tests.
It builds the control-logic test with `GIMBAL_MOTION_ENABLED=1`, then separately
builds the default locked configuration.

When using MSYS2/MinGW GCC from a repository path that contains non-ASCII
characters, pass an ASCII-only absolute build directory so the Windows linker
can create the `.exe` files reliably:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1 `
  -BuildDirectory <ascii-build-dir>
```

If the machine only has Keil MDK / ARMCLANG and no runnable host compiler, use
compile-only checks:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1 `
  -ArmclangPath <path-to-armclang.exe>
```

This verifies the same sources for Cortex-M0+ with `-Wall -Werror`, but it does
not execute the host-side assertions.
