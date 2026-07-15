# AGENTS.md

## Project scope
This is the SeekFree MSPM0G3507 library V3.3.4 adapted for the LCSC TianMengXing MSPM0G3507 board and Keil MDK.

## Hardware invariants
- Board LED: PB22, active high.
- User key: PB21, active low with pull-up.
- Debug UART0: PA10 TX, PA11 RX, connected to on-board CH340E.
- On-board SPI Flash: PB6 CS#, PB7 MISO, PB8 MOSI, PB9 SCK. Keep PB6 high unless intentionally accessing Flash.
- SWD: PA19/PA20. BSL: PA18.
- HFXT: PA5/PA6 40 MHz. LFXT: PA3/PA4 32.768 kHz.

## Editing rules
- Preserve all upstream GPL-3.0 copyright headers.
- Do not replace the bundled SeekFree/TI SDK 2.04 files with TianMengXing SDK 2.02 generated files.
- Do not reintroduce the upstream A14 forced-low startup behavior.
- Avoid PB6-PB9 for unrelated peripherals unless the on-board Flash bus sharing is deliberate.
- Existing C files are automatically seen by Keil after save/reload. New C files must also be added to the Keil .uvprojx group.
- Prefer application modules under project/user or project/code. Avoid modifying generated ti_msp_dl_config.c/.h unless the .syscfg source is updated consistently.

## Validation order
1. Build with Keil ARM Compiler 6.
2. LED PB22.
3. UART0 over Type-C at 115200.
4. Key PB21.
5. Timers/PWM/ADC.
6. External peripherals.
