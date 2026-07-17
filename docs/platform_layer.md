# 天猛星平台硬件抽象层

```text
application
    |
    v
platform
    |
    v
zf_driver / TI DriverLib
    |
    v
hardware
```

`application` 只通过 `App_*` 钩子和 `app_config.h` 描述需求；`platform` 将其映射到天猛星硬件；逐飞 `zf_driver` 及其调用的 TI DriverLib 保持原样，不由本项目修改。

## 文件职责

| 文件 | 职责 |
| --- | --- |
| `application/platform/platform_uart.*` | `platform_uart_send()` / `platform_uart_receive()`，UART0/1/2 初始化、非阻塞接收、发送完成等待、RS485 整帧发送 |
| `application/platform/platform_gpio.*` | `platform_led_set()` / `platform_key_read()`，PB22 LED、PB21 低有效按键、PB17 RS485 DE/RE |
| `application/config/app_config.h` | 唯一硬件映射来源 |

## UART 映射

| 平台接口 | 逐飞索引 | TX / RX | 用途 |
| --- | --- | --- | --- |
| `PLATFORM_UART_DEBUG` | `UART_0` | PA10 / PA11 | CH340E 调试 |
| `PLATFORM_UART_VISION` | `UART_1` | PA8 / PA9 | MaixCAM Pro |
| `PLATFORM_UART_MOTOR` | `UART_2` | PB15 / PB16 | X42S RS485 |

平台层调用逐飞的 `uart_init()`、`uart_query_byte()`、`uart_write_buffer()`。接收使用 `uart_query_byte()`，不会启用或替换现有 UART 中断回调。

## RS485 发送时序

`platform_rs485_send()` 支持手动方向模块时的顺序固定为：

1. PB17 设为发送态。
2. 通过 UART2 发送整帧。
3. 等待 `DL_UART_isBusy(UART2)` 变为假，确认最后一个停止位已发送。
4. PB17 设为接收态。

X42S 驱动已有相同的 `App_Rs485SetTxEnable(true) -> App_MotorSend() -> App_Rs485SetTxEnable(false)` 调用链；其中 `App_MotorSend()` 已在平台层等待发送完成，因此不会在帧尾仍在发送时切换 PB17。

当前采购的 TTL-RS485 小模块为自动方向型，没有 DE/RE 引脚：PB17 不接模块，模块根据 UART2 的 TXD 自动切换方向。平台层仍保留 PB17 逻辑，以兼容未来更换为手动方向模块的情况。

## GPIO 映射

| 功能 | 接口 | 配置 |
| --- | --- | --- |
| LED | `platform_led_set()` / `platform_led_toggle()` | PB22，推挽输出，高电平点亮 |
| KEY | `platform_key_read()` | PB21，上拉输入，低电平按下 |
| RS485_DE | `platform_rs485_set_tx_enable()` | PB17，推挽输出，低=接收，高=发送 |

## SysConfig 检查

已只读检查生成工程中的 `SeekFree_MSPM0G3507_Opensource_Library/libraries/sdk/ti_config`：

- `.syscfg`、`ti_msp_dl_config.c` 和 `ti_msp_dl_config.h` 未生成 UART1、UART2、PA8/PA9、PB15/PB16 或 PB17 配置。
- 本次未修改任何 SysConfig 或生成文件。
- 平台层可用逐飞的运行时 `uart_init()` 配置 UART1 和 UART2；若后续工程改为完全依赖 SysConfig，必须先在 `.syscfg` 中补齐这些资源并重新生成。

## 集成要求

将四个 `application/platform/*.c` 文件加入目标 Keil `.uvprojx` 的应用分组，并确保 include path 能找到 `zf_common_headfile.h`。`app_main.c` 的初始化顺序为 System Init -> platform init -> UART init -> vision init -> gimbal/motor init；平台层的强符号会覆盖同名弱钩子。

逐飞库没有可直接读取的毫秒时基，`App_GetMillis()` 仍由后续定时器/SysTick 平台实现提供；在它接入前，视觉 500 ms 超时和 20 ms 云台周期不能进行真实时序验证。
