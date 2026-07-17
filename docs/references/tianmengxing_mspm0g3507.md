# 天猛星 MSPM0G3507 资料摘要

## 原始资料

- 开发板资料与模块移植代码：`E:\26diansai\天猛星开发板资料与移植代码`，约 1.67 GB。
- 关键小文件：`02-【MSPM0G3507】开源硬件` 下的引脚图、原理图；`06-【MSPM0G3507】官方资料` 下的数据手册、用户手册和硬件手册。
- 芯片官方数据手册：[MSPM0G3507](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf)。

## 本项目冻结资源

| 功能 | 资源 | 说明 |
| --- | --- | --- |
| 调试 | UART0，PA10/PA11 | CH340E + Type-C，永久保留 |
| 视觉 | UART1，PA8/PA9 | MaixCAM 通信 |
| 电机 | UART2，PB15/PB16 | X42S RS485 模块通信 |
| LED / KEY | PB22 / PB21 | KEY 为低电平有效 |
| PB17 | GPIO 预留 | 仅供未来手动 DE/RE RS485 模块；当前自动方向模块不连接 |

完整接口和冲突检查以 `docs/hardware_interface.md` 与 `docs/mspm0_uart_plan.md` 为准。`05-【MSPM0G3507】开发工具` 体积较大，只在安装烧录或调试工具时按需获取。
