# AGENTS.md

## 先读这个

本文件是给队友的 Codex 使用的仓库总指令。进入本仓库后，先阅读本文件，再阅读下面的关键文件：

1. `README.md`
2. `docs/hardware_interface.md`
3. `docs/software_architecture.md`
4. `docs/tianmengxing_environment_setup.md`
5. `firmware/mspm0g3507/tianmengxing/AGENTS.md`
6. `firmware/mspm0g3507/tianmengxing/VALIDATION.md`
7. `docs/first_board_bringup.md`
8. `docs/gimbal_parameter_confirmation.md`
9. `docs/stm32_f407_skystar_bringup.md`
10. `docs/tianmengxing_expansion_pin_map.md`
11. `docs/stm32_f407_environment_setup.md`
12. `docs/codex_prompts/stm32_f407_environment_setup_prompt.md`
13. `docs/verified_wiring_and_bringup_log.md`
14. `docs/maixcam2_color_tracking_bringup.md`

## 当前主线任务

本仓库用于 2026“数智中原”河南省大学生电子设计竞赛备赛。当前 MSPM0G3507 主线是：

```text
MaixCAM2
    |
    | UART1
    v
MSPM0G3507 天猛星
    |
    | UART3 + RS485
    v
X42S 闭环步进电机
    |
    v
云台控制
```

当前优先目标不是继续扩展架构，而是保持工程可编译、可上板、可安全联调。

已经实物验证成功的 MaixCAM2、天猛星、RS485、X42S 云台接线和现象记录在：

```text
docs/verified_wiring_and_bringup_log.md
```

## 分支和工程入口

默认公开分支：

```text
main
```

仓库已经包含生成好的完整 Keil 工程，队友可以直接打开：

```text
firmware/mspm0g3507/tianmengxing/full_project/MSPM0G3507_Library-TianMengXing-V3.3.4/SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

如果修改了根目录 `application/` 或天猛星适配脚本，应重新刷新完整工程：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_keil_project.ps1 `
  -SeekFreeSourceDir <seekfree-source-dir> `
  -OutputDir firmware/mspm0g3507/tianmengxing/full_project/MSPM0G3507_Library-TianMengXing-V3.3.4 `
  -KeilUv4Path <path-to-UV4.exe> `
  -Force
```

如果队友只是打开仓库内已有完整工程编译，不需要先执行这条命令。

STM32F407 天空星备用工程入口：

```text
firmware/stm32_f407/skystar_stdperiph_project/project/MDK(V5)/Project.uvprojx
```

该工程基于立创天空星 STM32F407VET6 标准库模板，当前用于先验证
MaixCAM ASCII 假数据接收链路。说明见：

```text
docs/stm32_f407_skystar_bringup.md
```

如果队友的 STM32F407 环境无法打开、编译或下载，先读：

```text
docs/stm32_f407_environment_setup.md
docs/codex_prompts/stm32_f407_environment_setup_prompt.md
```

## 硬件资源冻结

以下资源已经冻结，除非队长明确要求，否则不要改：

| 功能 | 资源 |
| --- | --- |
| UART0 调试 | A10 TX / A11 RX，板载 CH340E + Type-C |
| UART1 MaixCAM | A8 TX / A9 RX |
| UART3 X42S RS485 | B12 TX / B13 RX |
| RS485 方向控制 | B14 |
| OLED | B4 SCL / B5 SDA，软件 IIC |
| 小车双驱 | B10/B11 左电机，B15/B16 右电机 |
| 七路灰度 | A21 到 A27，数字输入 |
| LED | B22 |
| KEY | B21，低电平按下 |
| SWD | A19 / A20 |
| 板载 SPI Flash | B6 / B7 / B8 / B9，避免占用 |

统一硬件宏在：

```text
application/config/app_config.h
```

应用层代码禁止直接散写 `PA8`、`B12`、`B14`、`B15` 等硬件编号，应引用 `app_config.h` 和 platform 层接口。

## 代码修改原则

- 优先修改根目录 `application/`、`docs/`、`tools/`。
- 不要随意重构逐飞底层库、TI SDK、STM32 工程或智能车代码。
- 如果必须修改天猛星板级适配，优先改：

```text
firmware/mspm0g3507/tianmengxing/overlay/
firmware/mspm0g3507/tianmengxing/scripts/materialize.py
```

- `full_project/` 是为了降低队友打开工程成本而提交的完整工程副本。修改应用层时，优先改根目录 `application/`，再刷新 `full_project/`。
- 不要提交 Keil 编译产物，例如 `Objects/`、`Listings/`、`.hex`、`.axf`、`.map`、`.o`。
- 不要在仓库文档中写入某个人电脑上的真实绝对路径。用 `<seekfree-source-dir>`、`<generated-output-dir>`、`<path-to-UV4.exe>` 这类占位符。

## 安全状态

云台是自制 3D 打印结构，机械限位和方向还没实测。默认必须保持：

```c
GIMBAL_MOTION_ENABLED == 0U
```

这意味着初始化只会失能电机，视觉目标不会生成位置运动命令。只有完成 `docs/gimbal_parameter_confirmation.md` 中的 ID、方向、零点、软限位确认后，才允许考虑解锁小角度单轴运动。

## 常用验证命令

Host 逻辑测试：

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1 `
  -BuildDirectory <ascii-build-dir>
```

Keil 自动生成和编译：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\build_keil_project.ps1 `
  -SeekFreeSourceDir <seekfree-source-dir> `
  -OutputDir <generated-output-dir> `
  -KeilUv4Path <path-to-UV4.exe> `
  -Force
```

MaixCAM UART1 假视觉场景：

```powershell
python .\tools\serial_debug\serial_debug.py maixcam scenario timeout `
  --yaw 1.0 --pitch -0.5 --hold 20 --timeout-gap 0.7 `
  --port COMx --interval 0.05
```

X42S USB-RS485 安全检查：

```powershell
python .\tools\serial_debug\serial_debug.py x42s scenario safe-check `
  --id 1 --port COMx
```

## 当前未完成事项

- 尚未连接天猛星实板验证。
- 尚未连接真实 MaixCAM。
- 尚未连接真实 X42S 双轴云台。
- Yaw/Pitch 电机 ID、正方向、零点、软限位仍需实物确认。

硬件到位后，按 `docs/first_board_bringup.md` 的顺序做，不要第一次就全系统一起接。
