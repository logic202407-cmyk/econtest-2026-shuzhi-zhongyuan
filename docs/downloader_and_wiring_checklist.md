# 下载器与接线准备清单

本文用于硬件到位前准备下载器、线材和首板接线。当前项目主控为立创天猛星 MSPM0G3507，推荐使用 XDS110 通过 SWD 下载和调试。

## 1. 下载器选择

| 方案 | 结论 | 说明 |
| --- | --- | --- |
| XDS110 | 推荐 | TI 官方调试器，适合 MSPM0G3507，在 Keil 中配合 TI 设备包使用 |
| 板载 USB | 仅作 UART0 调试 | 天猛星 Type-C 连接 CH340E，对应 UART0，不等于 SWD 下载器 |
| J-Link | 备用 | 若已有可用 J-Link 可后续尝试，但当前不作为首选路线 |

优先把 XDS110 跑通。后续上板验证只需要一套稳定下载链路，不需要同时准备多种下载器。

## 2. XDS110 到天猛星 SWD

| XDS110 信号 | 天猛星/MSPM0G3507 | 备注 |
| --- | --- | --- |
| SWDIO | PA19 / SWDIO | 调试数据 |
| SWCLK | PA20 / SWCLK | 调试时钟 |
| GND | GND | 必须共地 |
| VTref / 3.3V Sense | 3.3V | 用于电平参考，按 XDS110 接口定义连接 |
| NRST | NRST | 可选，若 Keil 下载不稳定再接 |

注意：

- 不要用 XDS110 给整套电机系统供电。
- 主控板、RS485 模块、X42S 控制信号侧必须共地。
- 第一次下载前只接主控板和 XDS110，先不要接 MaixCAM、RS485 和电机。

## 3. Keil 中需要确认

| 检查项 | 要求 |
| --- | --- |
| Keil MDK | 已安装 MDK-ARM |
| TI 设备包 | 已安装 `TexasInstruments::MSPM0G1X0X_G3X0X_DFP` |
| 工程设备 | MSPM0G3507 |
| 编译器 | ARM Compiler 6 / ARMCLANG |
| 调试器 | 选择 XDS110 |
| 下载前编译 | `0 Error(s), 0 Warning(s)` |

如果 Keil 能编译但不能下载，先检查 XDS110 驱动、SWDIO/SWCLK 是否接反、VTref 是否连接、目标板是否上电。

## 4. UART 与 RS485 接线

### UART0 调试

| 天猛星 | 用途 |
| --- | --- |
| PA10 | UART0 TX，接板载 CH340E |
| PA11 | UART0 RX，接板载 CH340E |

UART0 固定作为调试串口，115200 8N1。上板后预期看到 `APP,INIT`、`VISION,FRAME`、`VISION,TIMEOUT` 等状态日志。

### UART1 接 MaixCAM Pro

| 天猛星 | MaixCAM Pro |
| --- | --- |
| PA8 / UART1 TX | MaixCAM RX |
| PA9 / UART1 RX | MaixCAM TX |
| GND | GND |

默认 115200 8N1。没有 MaixCAM 时，可先用 USB-TTL 向 UART1 发送仓库工具生成的假视觉帧。

### UART2 接 RS485 模块

| 天猛星 | TTL-RS485 模块 |
| --- | --- |
| PB15 / UART2 TX | RXD |
| PB16 / UART2 RX | TXD |
| GND | GND |
| 3.3V | VCC，若模块支持 3.3V |

如果使用自动方向 RS485 模块，PB17 可不接。若使用普通半双工 RS485 收发器，将 DE 和 `/RE` 短接后接 PB17：

| PB17 | 状态 |
| --- | --- |
| 0 | 接收 |
| 1 | 发送 |

## 5. RS485 到 X42S

| RS485 模块 | X42S |
| --- | --- |
| A+ | A+ |
| B- | B- |
| GND | 控制信号地 |

若通信无响应，优先检查：

- A/B 是否接反。
- X42S 是否为 X 固件自由协议。
- 波特率是否为 115200。
- 校验是否为固定 `0x6B`。
- 电机 ID 是否与 `application/config/app_config.h` 一致。

## 6. 到板后的最低风险顺序

1. 只接 XDS110 和天猛星，确认 Keil 可下载。
2. 只测 PB22 LED、PB21 KEY、UART0 调试输出。
3. 用 USB-TTL 向 UART1 发假 MaixCAM 帧，确认解析和 500 ms 超时。
4. 用 USB-RS485 单独测试一个 X42S，确认 ID、方向、零点。
5. 保持 `GIMBAL_MOTION_ENABLED == 0U`，让天猛星接入 RS485，确认启动时只发送失能命令。
6. 填完 `docs/gimbal_parameter_confirmation.md` 后，再解锁小角度单轴运动。

