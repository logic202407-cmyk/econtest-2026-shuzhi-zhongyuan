# 备赛资料速览

## 项目链路

`MaixCAM Pro -> UART1 -> MSPM0G3507 -> UART2 + 自动方向 RS485 -> X42S`

## 分工与必读资料

| 成员 | 负责内容 | 优先阅读 |
| --- | --- | --- |
| 井 | MaixCAM Pro、视觉算法、视觉数据发送 | `docs/maixcam_protocol.md`、`firmware/maixcam/main.py` |
| 凯 | STM32F407、OLED、按键、执行机构 | `docs/uart_protocol.md`、`docs/x42s_rs485_analysis.md` |
| 队长 | MSPM0G3507、采购、扩展板、系统集成 | `docs/hardware_interface.md`、`docs/platform_layer.md`、`firmware/mspm0g3507/tianmengxing/README.md` |

## 已冻结接口

| 功能 | 引脚/接口 | 说明 |
| --- | --- | --- |
| 调试串口 | UART0，PA10/PA11 | 板载 CH340E，禁止占用 |
| 视觉通信 | UART1，PA8/PA9 | MaixCAM：A19 TX -> PA9，A18 RX <- PA8，3.3 V TTL、共地 |
| 电机通信 | UART2，PB15/PB16 | PB15 TX -> RS485 TXD，PB16 RX <- RS485 RXD |
| RS485 方向 | 自动方向模块 | 当前不接 PB17；PB17 仅预留给手动 DE/RE 模块 |
| 板载资源 | PB22 / PB21 | LED / 低电平有效按键 |

## X42S 关键结论

- 当前使用 X 固件自由协议，默认 `115200 8N1`、固定校验字节 `0x6B`。
- 上电前确认电机屏幕中的固件类型、站号、波特率和供电范围。
- 详细帧格式与控制接口见 `docs/x42s_rs485_analysis.md`。

## 官方与大文件资料

- MaixCAM Pro：[官方页](https://wiki.sipeed.com/hardware/zh/maixcam/maixcam_pro.html)、[UART 文档](https://wiki.sipeed.com/maixpy/doc/en/peripheral/uart.html)。
- MSPM0G3507：[TI 数据手册](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf)、[逐飞原始库](https://gitee.com/seekfree/MSPM0G3507_Library)。
- 队内网盘或 Release 应共享：X42S 用户手册、Modbus 手册、STM32F407 例程、TI MSPM0G3507 例程、天猛星原理图与资料。
- 厂商原始资料体积约 5.7GB，不直接放入普通 Git 仓库；入口与版本说明见 `docs/references/README.md`。
