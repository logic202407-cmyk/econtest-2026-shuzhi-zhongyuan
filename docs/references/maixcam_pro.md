# MaixCAM Pro 资料摘要

## 原始资料

- 本机资料：`E:\26diansai\Sipeed官方下载资料\MaixCAM\MaixCAM`，约 176.9 MB。
- 官方资料：[MaixCAM Pro 页面](https://wiki.sipeed.com/hardware/zh/maixcam/maixcam_pro.html)、[MaixPy UART 文档](https://wiki.sipeed.com/maixpy/doc/en/peripheral/uart.html)。

## 本项目只需掌握

- MaixCAM Pro 外置，不焊接到主控底板。
- 使用 MaixCAM 的 `UART1`：`A19` 为 TX、`A18` 为 RX，IO 电平为 3.3 V TTL。
- 接线：`A19 TX -> MSPM0 PA9 RX`，`A18 RX <- MSPM0 PA8 TX`，两端必须共地。
- 发送给 MSPM0 的二进制帧格式、字节序和超时规则以 `docs/maixcam_protocol.md` 为准。
- 调试起点：`firmware/maixcam/main.py` 先发送假数据，再替换为真实视觉结果。

## 分工建议

井优先负责视觉算法、串口发送和帧率稳定性；主控侧只负责协议解析，不在 MaixCAM 资料中寻找电机控制方案。
