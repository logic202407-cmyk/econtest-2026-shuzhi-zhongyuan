# Prompt for Teammate Codex: Set Up STM32F407 Skystar Environment

Copy this prompt into the teammate's Codex task after cloning the repository.

```text
你现在负责帮我配置 26 电赛仓库里的 STM32F407 天空星备用工程环境。

请先阅读：

1. AGENTS.md
2. docs/stm32_f407_environment_setup.md
3. docs/stm32_f407_skystar_bringup.md
4. firmware/stm32_f407/skystar_stdperiph_project/README.md

目标：

- 不新建 CubeMX/HAL 工程。
- 使用仓库已有工程：
  firmware/stm32_f407/skystar_stdperiph_project/project/MDK(V5)/Project.uvprojx
- 帮我把 Keil / STM32F4 device pack / ST-Link 驱动 / ARM Compiler 环境配到能打开、编译、下载。
- 如果需要用图形界面操作 Keil、Pack Installer、设备管理器或浏览器，请直接使用 computer use。

请按以下顺序执行：

1. 检查本机是否安装 Keil uVision，并找到 UV4.exe。
2. 打开或指导打开 STM32F407 工程。
3. 如果出现 `Device not found STM32F407VETx`，安装或指导安装 `Keil::STM32F4xx_DFP`。
4. 检查 ARM Compiler 6 是否可用。
5. 检查 Debug 设置是否为 ST-Link Debugger / SWD。
6. 检查 Flash Download 是否有 `STM32F4xx 512kB Flash` 算法。
7. 编译工程，目标是 `0 Error(s)`。
8. 如果我接了 ST-Link 和天空星板，继续帮我下载并做 UART0 启动日志验证。

限制：

- 不要改天猛星 MSPM0G3507 工程。
- 不要修改智能车主线代码，除非只是为了修 STM32 工程编译错误。
- 不要提交 Keil 生成的 Objects、Listings、hex、axf、map 等产物。
- 不要把我电脑里的绝对路径写入仓库文档。

最后输出：

- Keil 路径
- STM32F4xx_DFP 是否安装
- ARM Compiler 版本
- 工程是否能打开
- 是否能编译
- 是否能下载
- 如果失败，给出下一步最小修复动作
```

