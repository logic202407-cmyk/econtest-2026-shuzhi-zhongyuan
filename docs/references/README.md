# 队友资料入口

本目录只保存资料索引、版本和获取方式，不复制厂商手册、逐飞原始库或大体积生成工程。这样克隆仓库后仍能明确获得什么、从哪里获得以及如何核验版本。

## 精简版资料

| 资料 | 摘要 |
| --- | --- |
| 项目总览 | `materials_summary.md` |
| MaixCAM Pro | `maixcam_pro.md` |
| 天猛星 MSPM0G3507 | `tianmengxing_mspm0g3507.md` |
| 逐飞库与天猛星工程 | `seekfree_library.md` |
| X42S 电机与例程 | `x42s_materials.md` |

## 共同资料

| 资料 | 用途 | 获取方式 |
| --- | --- | --- |
| MaixCAM-Pro 官方页 | 板卡能力、扩展接口 | [Sipeed 官方页](https://wiki.sipeed.com/hardware/zh/maixcam/maixcam_pro.html) |
| MaixCAM UART 文档 | UART1 自定义通信、A19/A18、3.3 V 电平 | [Sipeed UART 文档](https://wiki.sipeed.com/maixpy/doc/en/peripheral/uart.html) |
| MSPM0G3507 数据手册 | UART、GPIO、定时器和 IOMUX | [TI 官方数据手册](https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf) |
| 逐飞 MSPM0G3507 原始库 | 逐飞底层驱动与 Keil 示例 | [Gitee 官方仓库](https://gitee.com/seekfree/MSPM0G3507_Library) |
| 项目接口规范 | MaixCAM、MSPM0、X42S 的共同约定 | `docs/maixcam_protocol.md`、`docs/hardware_interface.md`、`docs/platform_layer.md` |

## 按分工获取

### 井：MaixCAM 与视觉

- 阅读 `docs/maixcam_protocol.md`，它是当前 MSPM0 云台链路的二进制协议基准。
- 使用 MaixCAM-Pro 的 `UART1`：`A19` 为 TX、`A18` 为 RX；所有 IO 按 3.3 V TTL 连接。
- 假数据发送脚本在 `firmware/maixcam/main.py`。

### 凯：STM32、显示与执行机构

- 早期 ASCII 联调协议在 `docs/uart_protocol.md`。
- X42S 通信结论、关键指令和 STM32 例程位置在 `docs/x42s_rs485_analysis.md`。
- 队内共享包应包含 `ZDT_X42S第二代闭环步进电机用户手册V1.0.5_260527.pdf` 和 `9.例程_STM32F407`；厂商资料不直接提交到本仓库。

### 队长：MSPM0、采购与系统集成

- 天猛星适配层与生成脚本在 `firmware/mspm0g3507/tianmengxing/`。
- 先下载逐飞原始库，再按该目录 `README.md` 运行 `scripts/materialize.py` 生成完整 Keil 工程。
- 应用层与逐飞底层的连接关系见 `docs/platform_layer.md`。
- 逐飞 `E05_pit_demo` 可作为 `App_GetMillis()` 的 1 ms 时基参考。

## 当前硬件结论

- MaixCAM 是外置设备：`A19 TX -> MSPM0 PA9 RX`，`A18 RX <- MSPM0 PA8 TX`，双方共地。
- 当前 RS485 小模块是自动方向型：UART2 的 `PB15 TX -> TXD`、`PB16 RX <- RXD`，模块接 3.3 V；PB17 不接该模块。
- PB17 仍在配置中保留，便于未来替换成需要 DE/RE 的手动方向 RS485 模块。
- X42S 使用 X 固件自由协议时，默认 `115200`、固定校验字节 `0x6B`；首次上电前从电机屏幕确认固件类型、地址和供电范围。

## 队内共享清单

以下资料应通过队内网盘或 GitHub Release 单独共享，并保持原文件名与版本号：

1. `ZDT_X42S第二代闭环步进电机用户手册V1.0.5_260527.pdf`
2. `ZDT闭环步进电机MODBUS协议使用说明V1.0.1_260401.pdf`
3. `9.例程_STM32F407`
4. `14.例程_TI_MSPM0G3507`
5. 天猛星原理图与开发板资料
6. 逐飞原始库 ZIP 或其官方下载地址

不要把带有厂商授权限制或来源不明的资料直接推送到公开仓库。
