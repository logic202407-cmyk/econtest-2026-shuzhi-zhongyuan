# TianMengXing Development Environment Setup

This guide lets a teammate reproduce the MSPM0G3507 TianMengXing Keil project
and lets Codex work on the tracked adaptation layer safely.

## 1. What This Environment Builds

The repository does not store the full SeekFree upstream library. It stores:

- TianMengXing board overlay and generation script:
  `firmware/mspm0g3507/tianmengxing/`
- Application code:
  `application/`
- Hardware, protocol, and architecture documentation:
  `docs/`

The generation script copies a separately downloaded SeekFree source tree to a
new output directory, applies the TianMengXing overlay, adds `application/` to
the Keil project, and patches the required SysConfig settings.

Do not edit or overwrite the downloaded upstream source tree. Do not commit a
generated full library directory to this repository.

## 2. Required Software

Install the following on Windows:

| Tool | Required version or setting | Purpose |
| --- | --- | --- |
| Keil MDK-ARM | MDK 5.39 or later | Open, compile, and download the project |
| ARM Compiler | ARM Compiler 6 / ARMCLANG | Current project is verified with ARMCLANG |
| TI Device Family Pack | `TexasInstruments::MSPM0G1X0X_G3X0X_DFP` 1.3.1 | MSPM0G3507 startup, headers, and device support |
| Python | Python 3.9 or later | Run `materialize.py` |
| Git | Current Git for Windows | Clone, commit, and push repository changes |
| Git Credential Manager | Included with current Git for Windows | GitHub HTTPS authentication |

The verified build used Keil MDK 5.39 with ARM Compiler 6.21. The installation
folder is computer-specific and is not part of this repository.

## 3. Install the TI Device Pack

1. Open Keil uVision.
2. Select **Pack Installer**.
3. Search for `MSPM0G1X0X_G3X0X_DFP`.
4. Install `TexasInstruments::MSPM0G1X0X_G3X0X_DFP` version `1.3.1`.
5. Confirm that `MSPM0G3507` appears under the installed Texas Instruments
   device pack.

If the Pack Installer cannot access the network, download the same `.pack`
file from the TI/Keil pack source and install it locally through Pack
Installer. Do not substitute an unrelated MSPM0 pack.

## 4. Obtain the SeekFree Upstream Source

Clone or download the official source without modifying it:

```powershell
git clone https://gitee.com/seekfree/MSPM0G3507_Library.git `
  <seekfree-source-dir>
```

If cloning is unavailable, download the archive from:

```text
https://gitee.com/seekfree/MSPM0G3507_Library/repository/archive/master.zip
```

Extract it to a new directory, for example:

```text
<seekfree-source-dir>
```

## 5. Generate the TianMengXing Keil Workspace

Clone this repository and switch to the active branch:

```powershell
git clone https://github.com/logic202407-cmyk/econtest-2026-shuzhi-zhongyuan.git
cd econtest-2026-shuzhi-zhongyuan
git switch feat/tianmengxing-seekfree-v3.3.4
```

Run the generator using a new output directory. The output must not be the
upstream source directory.

Recommended automated command:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_keil_project.ps1 `
  -SeekFreeSourceDir <seekfree-source-dir> `
  -OutputDir <generated-output-dir> `
  -KeilUv4Path <path-to-UV4.exe>
```

If `UV4.exe` is already in `PATH`, or `KEIL_UV4_PATH` points to it, the
`-KeilUv4Path` argument can be omitted. The script runs `materialize.py`, checks
the generated structure, verifies Keil project references and the frozen
UART/GPIO map, then runs a Keil rebuild.

Manual generation command:

```powershell
py .\firmware\mspm0g3507\tianmengxing\scripts\materialize.py `
  <seekfree-source-dir> `
  --output <generated-output-dir>
```

If `py` is unavailable, use the full path to a Python 3 interpreter. The
generated workspace must contain:

```text
MSPM0G3507_Library-TianMengXing-V3.3.4/
|- SeekFree_MSPM0G3507_Opensource_Library/
|  `- project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
`- Example/TianMengXing_Coreboard_Demo/
```

Open this file in Keil:

```text
SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

Select **Rebuild**. The verified configuration builds with:

```text
0 Error(s), 0 Warning(s)
```

The project uses ARMCLANG-compatible weak symbols and places the TianMengXing
board header after the SeekFree GPIO driver declaration. These compatibility
fixes are made automatically by `materialize.py`.

## 6. GitHub HTTPS Login and Push

Before pushing, authenticate Git Credential Manager once:

```powershell
git credential-manager github login
```

Complete the browser authorization with the GitHub account that has repository
write access. Then push the current branch:

```powershell
git push origin feat/tianmengxing-seekfree-v3.3.4
```

If Git reports an inaccessible unrelated global `.gitconfig`, use this command
only for the affected Git invocation:

```powershell
$env:GIT_CONFIG_GLOBAL='NUL'
$env:GIT_CONFIG_NOSYSTEM='1'
git -c credential.helper=manager push origin feat/tianmengxing-seekfree-v3.3.4
```

The explicit `credential.helper=manager` keeps Git Credential Manager enabled
while bypassing the broken global configuration.

## 7. How a Teammate's Codex Should Work

Open the repository root in Codex, then ask Codex to read these first:

1. `firmware/mspm0g3507/tianmengxing/AGENTS.md`
2. `docs/hardware_interface.md`
3. `docs/software_architecture.md`
4. `docs/tianmengxing_environment_setup.md`

Codex may directly edit and commit these tracked areas:

```text
application/
docs/
firmware/mspm0g3507/tianmengxing/overlay/
firmware/mspm0g3507/tianmengxing/scripts/materialize.py
```

Codex must not directly modify these areas for application work:

```text
Downloaded SeekFree upstream library
Generated libraries/zf_driver/ and libraries/sdk/
STM32 project files
MaixCAM and X42S protocol definitions without an agreed interface change
```

When adding a new application `.c` file, update the materializer's Keil project
file list so regenerated projects still include it. Then regenerate to a new
output directory and rebuild before committing.

## 8. Hardware Resource Baseline

The generated project and application framework assume the following frozen
mapping:

| Function | Peripheral or GPIO | Notes |
| --- | --- | --- |
| Debug serial | UART0, PA10 TX / PA11 RX | On-board CH340E and Type-C |
| MaixCAM Pro | UART1, PA8 TX / PA9 RX | Vision protocol |
| X42S RS485 | UART2, PB15 TX / PB16 RX | Motor free protocol |
| RS485 direction | PB17 | Low: receive; high: transmit |
| LED | PB22 | Board LED |
| Key | PB21 | Active low |
| On-board SPI Flash | PB6 to PB9 | Keep clear of unrelated use |

See `docs/hardware_interface.md` for the authoritative interface definition.

## 9. Pre-Hardware Checklist

Before the board is available, confirm:

- Keil rebuild completes with zero errors and zero warnings.
- `application/` appears as an `application` group in the Keil project.
- The TI MSPM0G3507 device pack is installed.
- UART0, UART1, UART2, and PB17 settings exist in the generated `.syscfg`.
- Git status is clean after commit and the branch is pushed.

When hardware arrives, follow the validation order in
`firmware/mspm0g3507/tianmengxing/AGENTS.md`: LED, debug UART, key, then
external UART and RS485 devices.
