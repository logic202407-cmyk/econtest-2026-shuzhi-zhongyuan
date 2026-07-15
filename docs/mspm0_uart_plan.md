# MSPM0G3507 天猛星 UART 资源规划

目标：天猛星 MSPM0G3507 同时保留调试口、接收 MaixCAM 视觉数据，并接入 X42S RS485 电机。

## 结论

本分配已冻结，统一接口规范见 `docs/hardware_interface.md`，应用层统一配置见 `application/config/app_config.h`。

| 用途 | UART | TX | RX | 参数 | 备注 |
|-|-|-|-|-|-|
| 调试日志 / Type-C CH340E | UART0 | PA10 | PA11 | 115200, 8N1 | 固定保留，不接外设 |
| MaixCAM Pro | UART1 | PA8 | PA9 | 115200, 8N1, ASCII | MaixCAM TX 接 PA9；PA8 可选接 MaixCAM RX |
| X42S RS485 | UART2 | PB15 | PB16 | 115200, 8N1, binary | PB15 -> RS485 DI，PB16 <- RS485 RO |

建议 RS485 方向控制：

| 信号 | 建议 GPIO | 说明 |
|-|-|-|
| DE/RE | PB17 | 普通 485 芯片使用；发送前置 1，发送完成后置 0 |

如果使用自动收发方向的 RS485 模块，可不接 DE/RE。

## 资源依据

逐飞 MSPM0G3507 V3.3.4 库中 UART 可选脚包括：

- UART0：`UART0_TX_A10` / `UART0_RX_A11`
- UART1：`UART1_TX_A8` / `UART1_RX_A9`
- UART2：`UART2_TX_B15` / `UART2_RX_B16`

天猛星适配层已固定：

- UART0 PA10/PA11：板载 CH340E + Type-C 调试串口。
- PB6/PB7/PB8/PB9：板载 SPI Flash，避免占用。
- PB21：用户按键。
- PB22：用户 LED。
- PA19/PA20：SWD。
- PA18：BSL。
- PA3/PA4、PA5/PA6：晶振相关。

逐飞资料还提示 A21、A23 等属于“尽量不要使用”的特殊功能脚，因此 UART2 不选 A21/A22 或 A23/A24 组合，优先选 PB15/PB16。

## 接线方案

### UART0 调试

不用外接线，直接通过天猛星 Type-C 连接电脑串口助手。

### UART1 接 MaixCAM

```text
MaixCAM TX  -> MSPM0 PA9  / UART1 RX
MaixCAM RX  <- MSPM0 PA8  / UART1 TX，可选
MaixCAM GND -> MSPM0 GND
```

首阶段 MaixCAM 只发送视觉帧时，最小接线为 TX、GND、供电。若要让主控发送 `SET_MODE`、`PING` 等命令，再接 PA8 到 MaixCAM RX。

### UART2 接 X42S RS485

```text
MSPM0 PB15 / UART2 TX -> RS485 DI
MSPM0 PB16 / UART2 RX <- RS485 RO
MSPM0 PB17 / GPIO     -> RS485 DE 与 /RE，可选
MSPM0 GND             -> RS485 模块 GND
RS485 A/B             -> X42S A/B
```

普通半双工 RS485 收发器建议把 DE 和 /RE 短接到 PB17：

- 接收态：PB17 = 0
- 发送态：PB17 = 1
- 每次发送完最后一个字节并确认 UART 不忙后，切回接收态

## 软件分层

| 层 | 职责 |
|-|-|
| UART0 debug | `printf`、状态日志、调试命令，不参与业务协议 |
| UART1 vision | `$...#` ASCII 状态机，解析 `docs/uart_protocol.md` 的 V1 帧 |
| UART2 motor | X42S 二进制自由协议，默认固定校验 `0x6B` |

中断原则：

- UART 中断只收字节、入环形缓冲或置完整帧标志。
- MaixCAM 的逗号分割和数值转换放主循环。
- X42S 的响应解析放主循环或轻量状态机，不在中断里控制电机。
- UART0 日志输出不能阻塞 UART1/UART2 接收。

## 初始化建议

逐飞库接口命名可按下列资源规划：

```c
debug_init(); // UART0 PA10/PA11, 115200

uart_init(UART_1, 115200, UART1_TX_A8, UART1_RX_A9);
uart_rx_interrupt(UART_1, 1);

uart_init(UART_2, 115200, UART2_TX_B15, UART2_RX_B16);
uart_rx_interrupt(UART_2, 1);
gpio_init(B17, GPO, GPIO_LOW, GPO_PUSH_PULL);
```

实际函数名以当前逐飞库头文件为准，资源分配不变。

应用层不直接散写 UART 和 GPIO 引脚，建议写法：

```c
#include "app_config.h"

uart_init(VISION_UART_INDEX, VISION_UART_BAUDRATE,
          VISION_UART_TX_PIN, VISION_UART_RX_PIN);

uart_init(MOTOR_UART_INDEX, MOTOR_UART_BAUDRATE,
          MOTOR_UART_TX_PIN, MOTOR_UART_RX_PIN);

gpio_init(RS485_DE_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
```

## 风险与备选

| 风险 | 处理 |
|-|-|
| PB15/PB16 在实板排针位置不方便接线 | 当前冻结方案不直接改动；如实测接线不可行，需要重新确认硬件资源 |
| PB17 被后续 PWM/舵机占用 | 当前冻结方案不允许占用；如必须调整，需要重新确认硬件资源 |
| MaixCAM 需要更高波特率 | UART1 可提高，但先保持 115200 完成第一阶段闭环 |
| X42S 切到 Modbus-RTU | UART2 物理层不变，协议层改 CRC16 与 Modbus 帧 |
