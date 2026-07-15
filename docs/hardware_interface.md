# MSPM0G3507 天猛星硬件接口规范

本文档冻结 2026“数智中原”备赛项目中天猛星 MSPM0G3507 的硬件资源分配。后续应用层代码必须引用 `application/config/app_config.h`，禁止在业务代码中直接散写 `PA8`、`PB15`、`PB17` 等硬件引脚。

## 1. 系统整体架构

```text
MaixCAM Pro
    |
    | UART1
    v
MSPM0G3507
    |
    | UART2
    v
RS485 收发器
    |
    v
X42S 闭环步进电机
```

| 模块 | 职责 |
|-|-|
| MaixCAM Pro | 负责视觉识别，输出 `$V,mode,cx,cy,w,h,D,x,angle,conf#` 视觉帧 |
| MSPM0G3507 天猛星 | 负责 UART 接收解析、按键/LED/OLED 等应用逻辑、云台/电机控制决策 |
| RS485 收发器 | 负责把 MSPM0 的 UART2 TTL 信号转换为 X42S 使用的半双工 RS485 总线信号 |
| X42S | 执行云台运动，接收位置、速度、使能、停止、读位置等控制指令 |

## 2. UART资源分配表

|接口|TX|RX|用途|备注|
|-|-|-|-|-|
|UART0|PA10|PA11|调试|板载 CH340E + Type-C，永久保留|
|UART1|PA8|PA9|MaixCAM|视觉通信，MSPM0 TX -> MaixCAM RX，MSPM0 RX <- MaixCAM TX|
|UART2|PB15|PB16|X42S|经 RS485 收发器接入电机|

默认参数：

|接口|波特率|数据格式|
|-|-|-|
|UART0|115200|8N1|
|UART1|115200|8N1|
|UART2|115200|8N1|

## 3. GPIO资源表

|功能|GPIO|说明|
|-|-|-|
|LED|PB22|板载 LED|
|KEY|PB21|低电平按下|
|RS485_DE|PB17|方向控制，发送为 1，接收为 0|

普通半双工 RS485 芯片建议把 DE 与 `/RE` 短接后连接到 PB17：

- 发送前：`PB17 = 1`
- 发送完成且 UART 空闲后：`PB17 = 0`
- 接收等待态：`PB17 = 0`

## 4. 禁止占用资源

以下资源已经冻结，禁止其他模块占用：

|资源|原因|
|-|-|
|PA10 / PA11|UART0 调试串口，连接板载 CH340E + Type-C|
|PA8 / PA9|UART1 MaixCAM 视觉通信|
|PB15 / PB16|UART2 X42S RS485 通信|
|PB17|RS485 方向控制|
|PB22|板载 LED|
|PB21|板载按键，低电平有效|

额外保留建议：

|资源|原因|
|-|-|
|PB6 / PB7 / PB8 / PB9|天猛星板载 SPI Flash，除非明确重构 Flash 功能，否则不要占用|
|PA19 / PA20|SWD 调试|
|PA18|BSL 相关功能|

## 5. 统一硬件配置层

统一配置文件位置：

```text
application/config/app_config.h
```

应用层代码应通过以下宏访问硬件资源：

|宏|含义|
|-|-|
|`DEBUG_UART` / `DEBUG_UART_INDEX`|调试串口 UART0|
|`VISION_UART` / `VISION_UART_INDEX`|MaixCAM 通信 UART1|
|`MOTOR_UART` / `MOTOR_UART_INDEX`|X42S 通信 UART2|
|`VISION_UART_TX_PIN` / `VISION_UART_RX_PIN`|MaixCAM UART1 引脚|
|`MOTOR_UART_TX_PIN` / `MOTOR_UART_RX_PIN`|X42S UART2 引脚|
|`LED_PIN`|板载 LED|
|`KEY_PIN`|板载按键|
|`RS485_DE_PIN`|RS485 方向控制|

底层驱动保持在逐飞库；应用层只依赖统一配置文件和平台适配函数。

## 6. 硬件资源静态检查

本轮静态检查依据：天猛星已完成适配层与 AGENTS 约束、已生成逐飞工程 SysConfig、TI MSPM0G350x IOMUX 定义、逐飞 UART 引脚枚举。天猛星板载连接以当前适配层中已核验的连接关系为准。

|检查项|结论|
|-|-|
|UART0 PA10/PA11|已由天猛星适配层确认为板载 CH340E + Type-C 调试串口|
|UART1 PA8/PA9|MSPM0G3507 IOMUX 支持 `UART1_TX` / `UART1_RX`，逐飞库存在 `UART1_TX_A8` / `UART1_RX_A9`|
|UART2 PB15/PB16|MSPM0G3507 IOMUX 支持 `UART2_TX` / `UART2_RX`，逐飞库存在 `UART2_TX_B15` / `UART2_RX_B16`|
|PB17|可作为 GPIOB DIO17 普通输出；注意 PB17 也有 `UART2_TX` 复用，本项目禁止把 PB17 配成 UART2 TX|
|PB15/PB16/PB17 与 SPI Flash|不占用 PB6/PB7/PB8/PB9，未发现与板载 SPI Flash 冲突|
|PB15/PB16/PB17 与板载外设|不占用 PB21 按键、PB22 LED、PA10/PA11 调试串口、PA19/PA20 SWD|
|SysConfig|当前生成工程 SysConfig 主要包含 PB22 LED，未发现与 UART1/UART2/PB17 的冲突；后续实现需要在工程配置或初始化代码中显式加入 UART1、UART2 和 PB17|

结论：基于当前静态资料，未发现硬件资源冲突。
