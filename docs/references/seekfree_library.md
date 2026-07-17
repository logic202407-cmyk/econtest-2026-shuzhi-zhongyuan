# 逐飞 MSPM0G3507 库与天猛星工程摘要

## 原始库与生成工程

- 逐飞原始库不纳入仓库；请从 [SeekFree MSPM0G3507 Library](https://gitee.com/seekfree/MSPM0G3507_Library) 获取。
- 天猛星生成工程由仓库中的 `firmware/mspm0g3507/tianmengxing/scripts/materialize.py` 在每台电脑本地生成。
- 仓库中的适配层与生成脚本：`firmware/mspm0g3507/tianmengxing/`。

## 工程入口

完整工程的 Keil 文件：

`SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx`

天猛星示例目录：

`Example/TianMengXing_Coreboard_Demo`

生成工程中应存在：

```text
SeekFree_MSPM0G3507_Opensource_Library/
├── Example/TianMengXing_Coreboard_Demo/
└── project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

Keil 打开 `.uvprojx` 后，先确认工程包含 SDK、逐飞驱动和示例源文件；应用层的 `application/platform/*.c` 仍需加入目标工程分组。

## 推荐学习顺序

1. `E01_gpio_demo`：LED、按键和普通 GPIO。
2. `E02_uart_demo`：串口初始化、收发与中断。
3. `E05_pit_demo`：1 ms 周期中断，可作为系统时基参考。
4. `E04_pwm_demo`：后续云台或执行机构 PWM 扩展。

可选继续阅读：`E03_adc_demo` 用于电流检测，`E06_exti_demo` 用于外部中断，`E08_flash_demo` 用于理解板载 Flash，`E10_printf_debug_log_demo` 用于调试日志。

## 本项目与逐飞库的边界

| 层级 | 位置 | 允许修改 |
| --- | --- | --- |
| 应用 | `application/` | 是，放协议、云台和业务逻辑 |
| 平台适配 | `application/platform/` | 是，放 UART/GPIO 到逐飞 API 的映射 |
| 天猛星适配脚本 | `firmware/mspm0g3507/tianmengxing/` | 谨慎修改，保持可再生成 |
| 逐飞驱动与 SDK | 生成工程内 `libraries/` 等 | 否，不做业务性修改 |

## 重新生成方法

原始库更新或需重建生成工程时，使用仓库中的脚本，而不是覆盖原始库：

```powershell
python firmware/mspm0g3507/tianmengxing/scripts/materialize.py <逐飞原始库路径> --output <新生成目录>
```

生成后确认 Keil 工程路径、`Example/TianMengXing_Coreboard_Demo` 和 `.syscfg`/include 路径完整，再把应用层文件接入工程。

应用层代码位于 `application/`，通过 `application/platform/` 调用逐飞驱动；不要直接改动逐飞底层库或 SysConfig 生成文件。生成步骤、版本和适配说明见 `firmware/mspm0g3507/tianmengxing/README.md`。
