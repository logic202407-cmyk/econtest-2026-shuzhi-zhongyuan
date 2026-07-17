# 逐飞 MSPM0G3507 库与天猛星工程摘要

## 原始库与生成工程

- 逐飞原始库：`E:\26diansai\逐飞MSPM0G3507原始库\MSPM0G3507_Library-master`，约 302 MB；官方地址：[SeekFree MSPM0G3507 Library](https://gitee.com/seekfree/MSPM0G3507_Library)。
- 天猛星生成工程：`E:\26diansai\天猛星逐飞库适配与生成工程\MSPM0G3507_Library-TianMengXing-V3.3.4`，约 50 MB。
- 仓库中的适配层与生成脚本：`firmware/mspm0g3507/tianmengxing/`。

## 工程入口

完整工程的 Keil 文件：

`SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx`

天猛星示例目录：

`Example/TianMengXing_Coreboard_Demo`

## 推荐学习顺序

1. `E01_gpio_demo`：LED、按键和普通 GPIO。
2. `E02_uart_demo`：串口初始化、收发与中断。
3. `E05_pit_demo`：1 ms 周期中断，可作为系统时基参考。
4. `E04_pwm_demo`：后续云台或执行机构 PWM 扩展。

应用层代码位于 `application/`，通过 `application/platform/` 调用逐飞驱动；不要直接改动逐飞底层库或 SysConfig 生成文件。生成步骤、版本和适配说明见 `firmware/mspm0g3507/tianmengxing/README.md`。
