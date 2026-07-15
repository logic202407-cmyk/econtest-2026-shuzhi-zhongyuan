# 逐飞 MSPM0G3507 天猛星适配版 V3.3.4

本目录用于保存逐飞 MSPM0G3507 开源库面向立创·天猛星 MSPM0G3507 开发板的适配成果。

## 主要适配

- 板载 LED：`PA14` 调整为天猛星 `PB22`。
- 板载功能按键：使用 `PB21`，按下为低电平。
- 调试串口：保留 `UART0 / PA10 / PA11`，通过板载 CH340E 与 Type-C 使用。
- 板载 SPI Flash：`PB6` 作为 CS，上电拉高，避免误选中。
- PWM 示例避开板载 Flash 的 `PB9`，调整为 `PB20`。
- 附带 Keil 工程、核心板示例、移植说明、修改清单和供 Codex 使用的 `AGENTS.md`。

## 工程入口

解压完整适配包后，Keil 工程位于：

```text
SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

## 验证状态

已完成文件结构、工程引用、关键引脚和 C 代码静态检查。尚未在本环境中运行 Windows 版 Keil，也未连接天猛星实板。首次使用时应依次验证：

1. PB22 板载 LED 闪烁；
2. UART0 115200 串口输出；
3. PB21 按键输入；
4. PWM、ADC、定时器和其他外设例程。

## 完整包校验

```text
SHA-256: 10fd6aaa3f71ec090e2841661594ca30b1a791ac1091235928bb6c619e214a57
```
