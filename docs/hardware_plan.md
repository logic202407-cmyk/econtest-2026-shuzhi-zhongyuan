# 硬件方案

## 当前路线

| 方向 | 方案 |
| --- | --- |
| 视觉 | MaixCAM Pro |
| 不限制 M0 主控 | 立创天空星 STM32F407 |
| 限制 M0 主控 | 立创天猛星 MSPM0G3507 |
| 纯视觉拓展板 | 自研极简版 |
| 小车类拓展板 | 闲鱼现成板 |
| MaixCAM 安装 | 外置，通过 UART 和主控通信，不焊到底板上 |

## 接线原则

- MaixCAM TX 接主控 RX。
- MaixCAM RX 接主控 TX，若当前只发送视觉数据可先不接。
- MaixCAM 与主控必须共地。
- 优先使用 3.3 V TTL 串口。
- 供电链路预留足够电流余量。

## 天猛星 UART 分配

天猛星 MSPM0G3507 的硬件接口已冻结，完整规范见 `docs/hardware_interface.md`，应用层统一配置见 `application/config/app_config.h`。

| 用途 | UART | 引脚 |
| --- | --- | --- |
| 调试 | UART0 | PA10 TX / PA11 RX，板载 CH340E + Type-C |
| MaixCAM | UART1 | PA8 TX / PA9 RX |
| X42S RS485 | UART3 | B12 TX / B13 RX，B14 可做 DE/RE |

UART0 只做调试日志；MaixCAM 和 X42S 分开使用 UART1/UART3，避免视觉帧和电机二进制帧互相干扰。PA10/PA11、PA8/PA9、B12/B13、B14、PB21、PB22 禁止被其他模块占用。

## 纯视觉拓展板目标

- 提供主控、OLED、按键、电流检测接口。
- 提供 MaixCAM UART 接口与供电接口。
- 预留 X42S RS485 接口：B12、B13、B14、GND、5V/3V3。
- 保留下载、调试、备用 GPIO。
- 第一版以稳定、易焊、易改线为主。

## 小车/云台备用方向

- 采用闲鱼现成拓展板降低机械和功率驱动风险。
- 主控侧优先复用同一套 UART 协议解析和显示逻辑。
- 电机/舵机控制与视觉数据解耦，便于先用假数据调试。
