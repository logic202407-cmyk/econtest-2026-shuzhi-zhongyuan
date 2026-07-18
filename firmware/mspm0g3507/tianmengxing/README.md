# 逐飞 MSPM0G3507 天猛星适配版 V3.3.4

本目录保存逐飞 MSPM0G3507 开源库面向立创·天猛星 MSPM0G3507 开发板的适配层、生成脚本和验证记录。

为降低队友复现成本，本目录同时保存：

- `full_project/`：已经生成好的逐飞库天猛星完整 Keil 工程，可直接打开。
- `overlay/` 与 `scripts/materialize.py`：可审查、可复现的适配层和刷新脚本。

后续修改 `application/` 或板级适配时，优先修改仓库根目录下的源文件和适配脚本，再重新生成刷新 `full_project/`。

## 主要适配

- 板载 LED：逐飞默认 `PA14` 调整为天猛星 `PB22`，高电平点亮。
- 板载功能按键：使用 `PB21`，按下为低电平并启用上拉。
- 调试串口：保留 `UART0 / PA10 / PA11`，通过板载 CH340E 与 Type-C 使用。
- 板载 SPI Flash：`PB6` 为 CS，上电拉高；`PB7/PB8/PB9` 为 MISO/MOSI/SCK。
- GPIO 上电代码不再强制配置 `PA14`。
- PWM 示例从 `PB9` 调整到 `PB20`，避免和板载 Flash SCK 冲突。
- 提供供 Codex 使用的 `AGENTS.md`、硬件差异表、修改清单和静态验证记录。

## 直接打开完整工程

仓库内已包含完整工程：

```text
firmware/mspm0g3507/tianmengxing/full_project/MSPM0G3507_Library-TianMengXing-V3.3.4/
```

Keil 主工程入口：

```text
full_project/MSPM0G3507_Library-TianMengXing-V3.3.4/SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

队友 clone 仓库并切到 `feat/tianmengxing-seekfree-v3.3.4` 后，可以直接打开该 `.uvprojx`。

## 重新生成完整工作区

如果要从适配层重新生成完整工程，先下载并解压逐飞 `MSPM0G3507_Library-master.zip`，然后在本仓库根目录执行：

```bash
python firmware/mspm0g3507/tianmengxing/scripts/materialize.py \
  /path/to/MSPM0G3507_Library-master \
  --output ./build/MSPM0G3507_Library-TianMengXing-V3.3.4
```

Windows PowerShell 示例：

```powershell
python .\firmware\mspm0g3507\tianmengxing\scripts\materialize.py `
  <seekfree-source-dir> `
  --output <generated-output-dir>
```

生成后的正式 Keil 工程入口：

```text
SeekFree_MSPM0G3507_Opensource_Library/project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx
```

## Codex 使用方式

1. 在 Codex 中打开本 GitHub 仓库和分支 `feat/tianmengxing-seekfree-v3.3.4`。
2. 先阅读本目录的 `AGENTS.md`。
3. 修改板级资源时优先修改 `overlay` 和 `scripts/materialize.py`，不要直接覆盖逐飞或 TI 的版权头。
4. 如果只看工程或上板验证，直接打开 `full_project/` 下的 `.uvprojx`。
5. 如果修改了 `application/`，应重新生成刷新 `full_project/`，确保 Keil 工程分组同步。

## 验证状态

已完成文件结构、工程引用、关键引脚、host 逻辑测试和 Windows Keil/ARMCLANG 完整编译检查。尚未连接天猛星实板。首次使用时应依次验证：

1. PB22 板载 LED 闪烁；
2. UART0 以 115200 波特率通过 Type-C 输出；
3. PB21 按键输入；
4. PWM、ADC、定时器和其他外设例程。

## 已生成完整包的校验值

```text
SHA-256: 10fd6aaa3f71ec090e2841661594ca30b1a791ac1091235928bb6c619e214a57
```

该校验值对应早期生成的 `MSPM0G3507_Library-TianMengXing-V3.3.4.zip`。当前 GitHub 分支已直接保存完整工程源码；该 ZIP 不是必要入口。
