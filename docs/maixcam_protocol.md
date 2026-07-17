# MaixCAM Pro 到 MSPM0G3507 二进制协议

本文定义云台链路的 MaixCAM Pro 到 MSPM0G3507 协议。它替代早期 `$V,...#` ASCII 假数据格式；早期格式仍保留在 `docs/uart_protocol.md`，用于 STM32 联调兼容和历史参考。

## 1. 链路参数

| 项目 | 约定 |
| --- | --- |
| 物理接口 | UART1，MaixCAM TX -> PA9，MaixCAM RX <- PA8 |
| 串口参数 | 115200 baud，8N1，无流控 |
| 字节序 | 所有多字节整数均为小端序 |
| 角度单位 | `int16_t`，0.01 度 |
| 置信度单位 | `uint16_t`，0.01%，`10000` 表示 100.00% |
| 目标刷新 | 20 Hz，周期 50 ms |
| 失联判定 | 500 ms 未收到完整帧，或目标状态无效 |

MaixCAM 仅发送识别结果；MSPM0 不在此协议中下发 AI 控制指令。UART 中断或 DMA 仅接收数据，`application/vision/maixcam_protocol.c` 在主循环中解析。

## 2. 帧格式

```text
+--------+--------+-----+-----+----------+-------+
| Header | Header | CMD | LEN | DATA     | CHECK |
+--------+--------+-----+-----+----------+-------+
| 0xAA   | 0x55   | 1 B | 1 B | LEN bytes| 1 B   |
+--------+--------+-----+-----+----------+-------+
```

`CHECK` 为 `CMD + LEN + 所有 DATA 字节` 的低 8 位累加和。接收端对未知命令、长度不匹配或校验错误丢弃整帧，并继续搜索下一组 `0xAA 0x55`。

## 3. 命令

| CMD | 名称 | LEN | DATA |
| --- | --- | --- | --- |
| `0x01` | TARGET_FOUND | 6 | `yaw:int16`、`pitch:int16`、`confidence:uint16` |
| `0x02` | TARGET_LOST | 0 | 无 |
| `0x03` | HEARTBEAT | 6 | `uptime_ms:uint32`、`seq:uint16` |
| `0x04` | ERROR | 1 | `error_code:uint8` |

`TARGET_FOUND` 中的 yaw、pitch 是相对画面中心的误差，正负方向由相机安装方向联调后一次性确认。`TARGET_LOST` 立即使目标无效；即使持续收到心跳，云台也不应继续跟踪旧目标。

示例：yaw = 1.23 度、pitch = -0.45 度、confidence = 98.50%：

```text
AA 55 01 06 7B 00 D3 FF 7A 26 F4
```

## 4. 超时和故障处理

- 每 50 ms 发送一次 `TARGET_FOUND` 或 `TARGET_LOST`；每秒至少发送一次 `HEARTBEAT`。
- MSPM0 超过 500 ms 未得到有效目标，`Gimbal_Update()` 调用 `X42S_Stop()` 停止 yaw、pitch 两轴。
- 校验失败、长度错误和远端 `ERROR` 均不得复用上一帧目标数据。
- 收到下一帧合法 `TARGET_FOUND` 后，云台按比例控制自动恢复。

## 5. 代码对应

| 文件 | 职责 |
| --- | --- |
| `application/vision/maixcam_protocol.c` | 有界状态机、校验、字段解码、超时判断 |
| `application/gimbal/gimbal_control.c` | 读取最新目标，执行限幅比例控制和失目标停止 |
| `application/app_main.c` | 从 UART1 平台接收队列取字节并投入解析器 |
