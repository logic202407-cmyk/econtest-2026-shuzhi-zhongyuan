# MaixCAM + MSPM0 + X42S 软件架构

```text
MaixCAM2 --UART1--> vision/maixcam_protocol
                               |
                               v
                        gimbal/gimbal_control
                               |
                               v
                      motor/x42s_rs485 --UART3 + RS485--> X42S
                               ^
                               |
                         application/app_main
```

## 分层职责

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 配置层 | `application/config/app_config.h` | 冻结 UART、GPIO、协议时序、云台限位和电机默认参数 |
| 视觉协议层 | `application/vision/maixcam_protocol.*` | 解析有界二进制帧，不包含 AI 算法 |
| 云台控制层 | `application/gimbal/gimbal_control.*` | 将 yaw/pitch 误差转换为两个 X42S 位置目标，带比例、步进、机械限位和 500 ms 失目标保护 |
| 电机协议层 | `application/motor/x42s_rs485/x42s_rs485.*` | 封装 Emm 固件自由协议，不绑定具体 UART 驱动 |
| 应用编排层 | `application/app_main.c` | 初始化、有限预算轮询两个 UART、调用视觉解析和云台更新 |

## 主循环

`App_Run()` 在独立项目入口中调用；集成到逐飞 Keil 工程时，现有 `main.c` 初始化后调用 `App_Init()`，并在其 `while(true)` 中调用 `App_MainLoopOnce()`。不替换逐飞库的 `main.c`，避免修改底层适配层。

每轮最多处理 `APP_UART_POLL_BUDGET` 个 UART1 字节和同等数量的 UART3 字节，避免高频串口数据永久占用主循环。UART ISR/DMA 只写入接收 FIFO；平台的 `App_VisionReadByte()`、`App_MotorReadByte()` 从 FIFO 取数据。

## 平台适配契约

`application/app_main.c` 提供弱符号钩子，天猛星 Keil 工程在应用集成时覆盖它们：

| 钩子 | 约束 |
| --- | --- |
| `App_DebugUartInit()` | 使用 UART0，PA10/PA11，仅调试 |
| `App_VisionUartInit()` | 使用 UART1，PA8/PA9，115200 8N1 |
| `App_MotorUartInit()` | 使用 UART3，B12/B13，115200 8N1；B14 仅为手动方向 RS485 模块预留 |
| `App_MotorSend()` | 发送完成后才返回，确保 DE/RE 不会过早拉低 |
| `App_Rs485SetTxEnable()` | 手动方向模块时：`true` 设 B14 发送态，`false` 设接收态；自动方向模块不接 B14 |
| `App_GetMillis()` | 返回单调递增毫秒计数，允许 `uint32_t` 自然回绕 |

应用层不直接出现 PA8、PA9、B12、B13、B14 等引脚号；实际硬件映射只能从 `app_config.h` 取得。

## 故障策略

- 视觉帧校验或长度错误：丢弃帧，等待下一帧同步。
- 目标丢失或 500 ms 超时：两个 X42S 发送停止命令。
- UART3 回复：仅更新 X42S 最近位置/速度缓存，不阻塞控制循环等待回复。
- 电机配置、方向、零点和正负 yaw/pitch 需要在首次上电联调时校准，不在软件中假设机械方向。
