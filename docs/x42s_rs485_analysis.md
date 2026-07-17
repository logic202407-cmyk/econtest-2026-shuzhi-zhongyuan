# X42S RS485 接入分析

资料来源：`E:\26diansai\ZDT_XS系列第二代闭环步进电机资料`。本次只读遍历资料目录，并重点检查 `3. 说明书`、`9. 例程_STM32F407`、`13.例程_Modbus Poll`、`14.例程_TI_MSPM0G3507`。

## 一、X42S通信方式

X42S 支持 TTL / RS232 / RS485 / CAN 通讯。我们接入 MSPM0G3507 天猛星时，建议使用 RS485 物理层 + X 固件默认自由协议。

- 物理层：RS485 半双工总线，主控 UART 需要外接 RS485 收发器。
- 主控侧：项目规划使用 UART2，PB15 TX，PB16 RX；PB17 可作为 RS485 DE/RE 方向控制。资料包内 TI MSPM0G3507 例程使用 `UART0 / PA10 / PA11`，该例程的发送接口可参考，但本项目 UART0 已固定为调试口。
- 默认波特率：115200。
- 数据格式：8 数据位、无校验、1 停止位。
- 默认协议：自由协议，校验字节固定为 `0x6B`。
- Modbus：需要在电机菜单或上位机把通讯校验方式切到 Modbus-RTU，此时使用 CRC16。默认不是 Modbus。
- DE/RE：如果使用普通 RS485 芯片，需要 MSPM0 控制 DE/RE 方向；发送前拉到发送态，等待 UART 发送完成后切回接收态。若使用自动方向控制 RS485 模块，则软件可不接 DE/RE。

## 二、通信帧格式

默认自由协议帧格式：

|字段|说明|
|-|-|
|地址|1 字节，电机 ID，如 `0x01`|
|指令码|1 字节，例如使能 `0xF3`、速度模式 `0xF6`、位置模式 `0xFD`|
|数据内容|0 到多个字节，按大端序发送|
|校验字节|默认固定 `0x6B`|

典型例子：

- 失能 1 号电机：`01 F3 AB 00 00 6B`
- 使能 1 号电机：`01 F3 AB 01 00 6B`
- 立即停止 1 号电机：`01 FE 98 00 6B`
- 读取实时位置：`01 0F 6B`

注意：这套默认 X 固件自由协议没有独立“数据长度”字段，接收端按指令码识别长度。多电机命令 `0xAA` 例外，格式为：

`Addr AA LenH LenL <多个完整子命令> 6B`

如果切换到 Modbus-RTU，则帧包含 Modbus 地址、功能码、寄存器地址、寄存器数量/字节数、数据、CRC16，和上表不同。

## 三、关键控制指令

以下表格按 X 固件自由协议整理，数值均大端序。

|功能|指令|数据格式|
|-|-|-|
|使能|`0xF3`|`Addr F3 AB 01 snF 6B`，`snF=1` 表示等待多机同步|
|失能|`0xF3`|`Addr F3 AB 00 snF 6B`|
|回零|`0x9A`|`Addr 9A mode snF 6B`|
|设置单圈回零零点|`0x93`|`Addr 93 88 svF 6B`，`svF=1` 存储|
|位置控制|`0xFD`|`Addr FD dir accH accL decH decL velH velL pos3 pos2 pos1 pos0 raf snF 6B`，速度/位置按 0.1 单位放大|
|速度控制|`0xF6`|`Addr F6 dir accH accL velH velL snF 6B`，速度按 0.1 RPM 放大|
|力矩控制|`0xF5`|`Addr F5 sign rampH rampL curH curL snF 6B`|
|停止|`0xFE`|`Addr FE 98 snF 6B`|
|多机同步开始|`0xFF`|`Addr FF 66 6B`|
|读取实时转速|`0x0E`|`Addr 0E 6B`|
|读取实时位置|`0x0F`|`Addr 0F 6B`|

单位和符号：

- `dir/sign`：通常 `0` 为正/CW，`1` 为负/CCW。
- X 固件速度命令按 0.1 RPM 输入，`120.0 RPM` 发送为 `1200`。
- X 固件位置角度按 0.1 度输入，`360.0 deg` 发送为 `3600`。
- `raf`：`0` 相对上一次目标，`1` 绝对位置，`2` 相对当前实时位置。

## 四、STM32代码分析

重点例程：

- `9. 例程_STM32F407/HAL库/X固件模式/串口通讯`
- `9. 例程_STM32F407/HAL库/X固件模式/CAN通讯`
- `14.例程_TI_MSPM0G3507/X固件模式/串口通讯`

STM32 HAL 串口例程结构：

- `Src/main.c`：初始化 `MX_GPIO_Init()`、`MX_DMA_Init()`、`MX_USART1_UART_Init()`，打开 UART IDLE 中断和 DMA 接收，延时 500 ms 后调用控制函数。
- `Src/usart.c`：USART1，115200，8N1，PA9 TX / PA10 RX，DMA 循环接收，DMA 普通发送。
- `Src/stm32f4xx_it.c`：串口中断内根据 IDLE 事件截取一帧数据到 `rxCmd`。
- `Src/X_V2.c`、`Inc/X_V2.h`：真正有价值的协议封装层，核心逻辑是组包后发送。

可以迁移到 MSPM0G3507 的部分：

- `X_V2.c/.h` 的命令组包逻辑可以迁移。
- `HAL_UART_Transmit_DMA()` 不能直接迁移，应替换成 MSPM0 DriverLib 发送函数。
- STM32 DMA + IDLE 接收思路可参考，但 MSPM0 资料包已经给了更简单的 UART RX 中断 + FIFO 实现。
- CAN 例程的业务命令码与串口一致，但物理和收发 API 不同；本次 RS485 接入不需要优先迁移 CAN。

MSPM0 官方例程关键信息：

- `empty.syscfg` 配置 `UART0`、`PA10 TX`、`PA11 RX`、115200；本项目迁移时改用 `UART2`、`PB15 TX`、`PB16 RX`。
- `ti_msp_dl_config.c` 配置 8 数据位、无校验、1 停止位。
- `bsp/usart.c` 通过 `DL_UART_Main_transmitData()` 逐字节发送，通过 UART RX 中断入 FIFO。
- `bsp/X_V2.c` 已把 STM32 的 `HAL_UART_Transmit_DMA()` 替换为 `usart_SendCmd()`。

## 五、设计MSPM0驱动框架

已生成：

```text
application/motor/x42s_rs485/
├── x42s_rs485.c
└── x42s_rs485.h
```

设计原则：

- 驱动层只负责组 X42S 自由协议帧，不直接绑定某个 UART 外设。
- 驱动支持 `X42S_SetPortOps()` 注入发送与方向控制回调；未注入时兼容弱符号 `X42S_PortSend()` / `X42S_PortSetTxEnable()`。两种方式都不把驱动绑定到具体 UART。
- 天猛星平台回调使用 `MOTOR_UART_INDEX / MOTOR_UART_TX_PIN / MOTOR_UART_RX_PIN`，方向控制使用 `RS485_DE_PIN`。
- 发送回调必须在 UART2 最后一个字节真正发送完成后才返回；驱动在回调返回后立即将 DE/RE 切回接收态，避免 PB17 过早拉低截断帧尾。
- 硬件资源统一从 `application/config/app_config.h` 获取，业务代码禁止直接散写 `PB15`、`PB16`、`PB17`。
- 当前已提供使能、失能、位置、速度、停止、读位置接口；后续硬件跑通后再补完整回零、读速度、同步控制和 Modbus-RTU。

接线建议：

- MSPM0G3507 PB15 / UART2 TX -> RS485 模块 DI。
- MSPM0G3507 PB16 / UART2 RX <- RS485 模块 RO。
- MSPM0G3507 PB17 / GPIO -> RS485 模块 DE/RE，若模块没有自动方向控制。
- RS485 A/B -> X42S RS485 A/B，GND 共地。

下一步硬件验证：

1. 确认 X42S 菜单中串口波特率为 115200，通讯校验方式为自由协议固定 `0x6B`。
2. 用 USB-RS485 工具先发 `01 F3 AB 00 00 6B`，确认电机返回应答并失能。
3. 天猛星 UART2 发送同一帧，逻辑分析仪确认 TX 字节和 DE/RE 时序。
4. 接收返回帧后再测试 `01 0F 6B` 读取实时位置。
5. 最后测试低速速度模式和小角度位置模式，避免首次上电大幅运动。
