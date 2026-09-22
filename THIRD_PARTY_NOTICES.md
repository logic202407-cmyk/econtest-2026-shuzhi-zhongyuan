# Third-Party Components

The root `LICENSE` applies to original project code and documentation created
by this repository's contributors. Bundled third-party code remains subject to
its own license terms and copyright notices.

| Component | Location | License boundary |
| --- | --- | --- |
| SeekFree MSPM0G3507 Library V3.3.4 | `firmware/mspm0g3507/tianmengxing/full_project/` | GPL notices embedded in upstream source files; original notices are preserved |
| Texas Instruments MSPM0 SDK content | Bundled under the generated MSPM0 workspace | TI and component-specific notices embedded in upstream files |
| STM32F4 Standard Peripheral Library | `firmware/stm32_f407/skystar_stdperiph_project/libraries/STM32F4xx_StdPeriph_Driver/` | See the bundled `LICENSE.txt` |
| CMSIS STM32F4 device files | `firmware/stm32_f407/skystar_stdperiph_project/libraries/CMSIS/Device/ST/STM32F4xx/` | BSD-3-Clause; see the bundled `LICENSE.txt` |
| MCUboot test assets included by the TI SDK | MSPM0 workspace SDK subtree | Upstream test-only keys and Apache-licensed tooling; not production credentials |

The repository does not claim ownership of third-party board support packages,
vendor SDKs, or their trademarks. When redistributing a subset of the project,
retain the corresponding source headers and bundled license files.
