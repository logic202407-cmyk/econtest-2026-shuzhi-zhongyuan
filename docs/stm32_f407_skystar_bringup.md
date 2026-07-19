# STM32F407 天空星标准库工程联调说明

## 1. 工程入口

仓库内已经加入立创天空星 STM32F407VET6 标准库空白模板，并接入第一版
MaixCAM 串口接收 Demo：

```text
firmware/stm32_f407/skystar_stdperiph_project/project/MDK(V5)/Project.uvprojx
```

该工程基于立创标准库模板，包含 CMSIS 和 STM32F4xx StdPeriph Driver。当前不要把
CubeMX/HAL 文件混进这个工程。

## 2. 串口分配

| 功能 | 外设 | 引脚 | 说明 |
| --- | --- | --- | --- |
| 调试输出 | USART1 | PA9 TX / PA10 RX | `printf` 输出 |
| MaixCAM 输入 | USART2 | PA2 TX / PA3 RX | 接收视觉假数据 |

接线：

```text
MaixCAM TX  -> STM32 PA3 / USART2 RX
MaixCAM RX  <- STM32 PA2 / USART2 TX
MaixCAM GND -> STM32 GND
```

第一阶段只接收 MaixCAM 数据时，可以只接 `MaixCAM TX -> PA3` 和共地。

## 3. 当前已实现

- 使用立创天空星 STM32F407 标准库 Keil 模板。
- `USART1` 保持为调试输出。
- 新增 `USART2` 初始化和接收中断。
- 新增 ASCII 视觉帧解析器：
  - V1：`$V,ver,seq,mode,cx,cy,w,h,distance,size,angle,valid#`
  - 兼容旧格式：`$V,mode,cx,cy,w,h,D,x,angle,conf#`
- 主循环打印解析结果。
- 超过 500 ms 没有有效帧时输出超时状态。

## 4. 测试方法

1. 用 Keil 打开 `Project.uvprojx`。
2. 确认已安装 `Keil.STM32F4xx_DFP.2.17.1.pack`。
3. 编译并下载到天空星 STM32F407。
4. 串口助手打开 USART1 调试串口，参数 `115200, 8N1`。
5. 用 USB-TTL 或 MaixCAM 向 USART2 发送：

```text
$V,1,1,1,320,240,80,80,1500,80,0,1#
```

或旧格式：

```text
$V,1,320,240,80,80,150.0,8.0,0.0,0.99#
```

调试串口应输出类似：

```text
VISION OK seq=1 mode=1 cx=320 cy=240 w=80 h=80 dist=150.0cm size=8.0cm angle=0.0deg
VISION LINK CONNECTED
```

停止发送超过 500 ms 后，应输出：

```text
VISION LINK TIMEOUT
```

## 5. 下一步

等基础串口链路跑通后，再把 ZDT 的 STM32F407 标准库 X 固件串口例程中
`X_V2.c/.h` 的 RS485/X42S 控制思路迁入本工程。不要直接把 ZDT 整个工程覆盖到
天空星模板上。
