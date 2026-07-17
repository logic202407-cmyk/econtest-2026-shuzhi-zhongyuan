# econtest-2026-shuzhi-zhongyuan

2026“数智中原”河南省大学生电子设计竞赛备赛仓库。

本队优先准备纯视觉类题目，备用准备小车/云台控制类题目。当前最小目标是在 **2026 年 7 月 22 日前** 跑通：

```text
MaixCAM Pro -> UART -> STM32F407 / MSPM0G3507 -> OLED显示 / 按键 / 电流检测
```

## 硬件路线

| 模块 | 当前选择 |
| --- | --- |
| 视觉 | MaixCAM Pro |
| 不限制 M0 时主控 | 立创天空星 STM32F407 |
| 限制 M0 时主控 | 立创天猛星 MSPM0G3507 |
| 纯视觉拓展板 | 自研极简版 |
| 小车类拓展板 | 闲鱼现成板 |
| MaixCAM 安装方式 | 外置，通过 UART 和主控通信，不焊到底板上 |

## 统一串口协议

MaixCAM 向主控发送视觉识别结果：

```text
$V,mode,cx,cy,w,h,D,x,angle,conf#
```

示例：

```text
$V,1,320,240,80,80,150.0,8.0,0.0,0.99#
```

字段说明见 [docs/uart_protocol.md](docs/uart_protocol.md)。

## 目录结构

```text
.
├── README.md
├── docs/
├── firmware/
│   ├── maixcam/
│   ├── stm32_f407/
│   └── mspm0_g3507/
├── hardware/
│   ├── vision_extension_board/
│   └── car_extension_board/
├── tools/
│   └── serial_debug/
└── assets/
```

## 队伍分工

- 井：MaixCAM Pro、视觉算法、UART 发送数据
- 凯：STM32F407、显示、按键、舵机/电机控制
- 队长：MSPM0G3507、采购、视觉拓展板、系统集成

## 当前任务

1. MaixCAM Pro 周期发送假数据。
2. STM32F407 / MSPM0G3507 接收并解析 `$V,...#` 数据帧。
3. OLED 显示关键字段。
4. 按键切换模式。
5. 电流检测链路跑通。

## 资料笔记

- [MSPM0G3507 天猛星资料阅读笔记](docs/mspm0_g3507_resources.md)
- [队友资料入口](docs/references/README.md)
