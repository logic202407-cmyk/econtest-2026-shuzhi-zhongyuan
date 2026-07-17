# MSPM0G3507 天猛星资料阅读笔记

本机资料路径：

```text
本节依据天猛星开发板资料整理；原始资料目录不纳入仓库。
```

## 资料结构

| 路径 | 内容 | 当前用途 |
| --- | --- | --- |
| `立创·天猛星MSPM0G3507开发板资料/02-【MSPM0G3507】开源硬件` | 原理图、引脚图 PDF | 确认 UART、I2C、ADC、按键、LED、SWD 引脚 |
| `立创·天猛星MSPM0G3507开发板资料/03-【MSPM0G3507】开源软件` | Keil / CCS 基础例程 zip | 作为最小工程和外设初始化参考 |
| `立创·天猛星MSPM0G3507开发板资料/04-【MSPM0G3507】文档教程` | Keil / CCS-Theia 入门 HTML 手册 | 配环境、导入工程、下载调试 |
| `立创·天猛星MSPM0G3507开发板资料/05-【MSPM0G3507】开发工具` | MSPM0 SDK、SysConfig、DFP、J-Link、串口工具、CH340 驱动 | Windows 开发环境准备 |
| `立创·天猛星MSPM0G3507开发板资料/06-【MSPM0G3507】官方资料` | 数据手册、用户手册、硬件手册 | 查寄存器、供电、ADC、外设细节 |
| `立创·天猛星MSPM0G3507开发板资料/07-【MSPM0G3507】模块移植手册` | CCS 模块移植手册 | 把模块例程搬进主工程 |
| `立创·天猛星MSPM0G3507开发板【模块移植代码】` | 显示、控制、传感器、无线通信模块例程 | OLED、OpenMV/MaixCAM UART、舵机/电机备用 |

## 当前最有用的例程

| 目标 | 参考资料 |
| --- | --- |
| UART 接收 MaixCAM 数据 | `03-【MSPM0G3507】开源软件/Keil环境例程.zip` 中 `05_uart`；模块代码中 `无线通信类/OpenMV4摄像头` |
| OLED 显示 | `模块移植代码/显示类/0.96寸IIC单色屏` 或 `1.3寸单色OLED显示屏` |
| 按键 | `03-【MSPM0G3507】开源软件/Keil环境例程.zip` 中 `03_key` |
| 电流检测 ADC | `03-【MSPM0G3507】开源软件/Keil环境例程.zip` 中 `08_adc` |
| 基础 I2C | `03-【MSPM0G3507】开源软件/Keil环境例程.zip` 中 `10_i2c` |

## 已确认引脚

| 功能 | 例程/原理图引脚 | 备注 |
| --- | --- | --- |
| UART0 TX | PA10 | 板载调试/基础串口例程使用 |
| UART0 RX | PA11 | 板载调试/基础串口例程使用 |
| UART1 TX | PA8 | OpenMV4 模块例程使用，可作为外部视觉串口 |
| UART1 RX | PA9 | OpenMV4 模块例程使用，可作为外部视觉串口 |
| UART2 TX | PB15 | 推荐接 X42S RS485 模块 DI |
| UART2 RX | PB16 | 推荐接 X42S RS485 模块 RO |
| RS485 DE/RE | PB17 | 普通半双工 485 模块方向控制，自动方向模块可不接 |
| OLED I2C0 SDA | PA0 | 0.96 寸 IIC OLED 例程使用 |
| OLED I2C0 SCL | PA1 | 0.96 寸 IIC OLED 例程使用 |
| ADC0 CH0 | PA27 | `08_adc` 例程使用，可接电流检测模拟输出 |
| 功能按键 | PB21 | 输入上拉，按下为低电平 |
| 用户 LED | PB22 | 可做状态指示 |
| SWDIO | PA19 | 调试下载 |
| SWCLK | PA20 | 调试下载 |
| BSL | PA18 | Bootloader 相关 |

## 建议联调路线

1. 用 Keil 或 CCS 打开基础例程，先跑 `01_led` / `03_key`，确认下载链路和板卡正常。
2. 跑 `05_uart`，确认 UART0 打印和串口助手可用。
3. 若 MaixCAM 接主控，优先参考 OpenMV4 例程的 UART1 中断接收方式，使用 PA8/PA9 连接外部视觉模块。
4. 跑 OLED I2C 例程，确认 PA0/PA1 + OLED 地址 `0x3C` 可显示。
5. 跑 `08_adc`，把电流检测模块模拟输出接 PA27，先显示原始 ADC 和换算电压。
6. 合并时保持三条链路互不阻塞：UART 中断收帧，主循环解析协议，OLED 定时刷新。

## UART 资源规划

| 用途 | UART | 引脚 | 处理 |
| --- | --- | --- | --- |
| 调试 | UART0 | PA10 TX / PA11 RX | 保留给板载 CH340E 和 Type-C，不外接业务模块 |
| 视觉 | UART1 | PA8 TX / PA9 RX | 接 MaixCAM Pro，解析 `$...#` ASCII 协议 |
| 电机 | UART2 | PB15 TX / PB16 RX | 接 X42S RS485 收发器，PB17 可做 DE/RE |

UART2 也支持 A21/A22、A23/A24 等组合，但逐飞资料提示 A21、A23 属于“尽量不要使用”的特殊功能脚；因此本项目优先使用 PB15/PB16，避开 PB6-PB9 板载 SPI Flash、PB21 按键、PB22 LED 和 SWD/晶振相关引脚。

## MaixCAM 接线建议

| MaixCAM | MSPM0G3507 |
| --- | --- |
| TX | PA9 / UART1 RX |
| RX | PA8 / UART1 TX，可选 |
| GND | GND |
| 3V3/5V | 按 MaixCAM 实际供电要求单独确认 |

若只接收 MaixCAM 视觉数据，最小接线为 MaixCAM TX、GND、供电。注意两端必须共地，串口电平优先保持 3.3 V TTL。

## X42S RS485 接线建议

| RS485 模块 | MSPM0G3507 |
| --- | --- |
| DI | PB15 / UART2 TX |
| RO | PB16 / UART2 RX |
| DE 与 /RE | PB17，可选 |
| GND | GND |
| A/B | X42S A/B |

X42S 默认自由协议为二进制帧，校验字节固定 `0x6B`，不要和 MaixCAM 的 ASCII 帧解析共用同一个状态机。

## 协议实现提醒

我们项目协议是：

```text
$V,mode,cx,cy,w,h,D,x,angle,conf#
```

OpenMV4 例程默认找 `[` 和 `]`，不能直接照搬协议解析逻辑；适合复用的是 UART 中断、接收缓存、完成标志这些结构。解析时应改为：

1. 看到 `$` 开始缓存。
2. 看到 `#` 结束一帧。
3. 检查 `$V,`。
4. 用逗号分割并转换字段。
5. 字段数量或数值转换失败就丢弃该帧。
