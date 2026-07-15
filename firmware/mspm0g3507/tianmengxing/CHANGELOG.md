# 天猛星适配修改清单

- `zf_common_board_tianmengxing.h`：新增天猛星板级引脚和初始化函数。
- `zf_common_headfile.h`：引入天猛星板级头文件。
- `zf_driver_gpio.c`：移除 PA14 强制低电平；增加 PB22 关闭和 PB6 Flash CS# 拉高。
- `zf_common_debug.h`：调试串口仍为 UART0 PA10/PA11，对应板载 CH340E。
- `ti_config/*.syscfg/.c/.h`：LED 从 PA14 同步为 PB22。
- 正式空工程 `main.c`：改为 LED、按键、串口联合验证。
- Coreboard_Demo：适配 LED、板载按键，并将 PWM 示例从 PB9 改到 PB20，避开板载 Flash SCK。

完整工作区由 `scripts/materialize.py` 基于逐飞 V3.3.4 上游源码生成，避免在本仓库重复提交整套上游源码。
