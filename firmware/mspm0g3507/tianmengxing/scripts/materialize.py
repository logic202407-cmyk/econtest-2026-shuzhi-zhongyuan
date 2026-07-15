#!/usr/bin/env python3
"""Create a TianMengXing-adapted SeekFree MSPM0G3507 V3.3.4 workspace.

Example:
    python materialize.py /path/to/MSPM0G3507_Library-master \
        --output ./build/MSPM0G3507_Library-TianMengXing-V3.3.4

The repository stores only the TianMengXing overlay and this deterministic
materializer. Original SeekFree sources and copyright notices remain upstream.
"""
from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path


def copy_overlay(overlay_root: Path, output_root: Path) -> None:
    for source in overlay_root.rglob("*"):
        if not source.is_file():
            continue
        relative = source.relative_to(overlay_root)
        destination = output_root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def replace_required(path: Path, old: str, new: str, count: int = 1) -> None:
    text = path.read_text(encoding="utf-8")
    if old not in text:
        raise RuntimeError(f"Expected V3.3.4 text was not found in {path}: {old[:80]!r}")
    path.write_text(text.replace(old, new, count), encoding="utf-8")


def patch_common_library(tree_root: Path) -> None:
    libraries = tree_root / "libraries"
    common = libraries / "zf_common"
    driver = libraries / "zf_driver"
    ti_config = libraries / "sdk" / "ti_config"

    headfile = common / "zf_common_headfile.h"
    marker = "//===================================================芯片外设驱动层==================================================="
    include = (
        "//===================================================天猛星板级定义===================================================\n"
        "#include \"zf_common_board_tianmengxing.h\"\n"
        "//===================================================天猛星板级定义===================================================\n\n"
    )
    text = headfile.read_text(encoding="utf-8")
    if "zf_common_board_tianmengxing.h" not in text:
        if marker not in text:
            raise RuntimeError(f"Cannot locate insertion marker in {headfile}")
        headfile.write_text(text.replace(marker, include + marker, 1), encoding="utf-8")

    gpio_file = driver / "zf_driver_gpio.c"
    gpio_text = gpio_file.read_text(encoding="utf-8")
    pattern = re.compile(r"void gpio_set_a0_a1_output \(void\)\n\{.*?\n\}", re.DOTALL)
    replacement = """void gpio_set_a0_a1_output (void)
{
    gpio_init(A0, GPO, GPIO_LOW, GPO_PUSH_PULL);
    gpio_init(A1, GPO, GPIO_LOW, GPO_PUSH_PULL);

    // 天猛星板级安全状态：PB22 板载 LED 低电平熄灭，PB6 板载 SPI Flash CS# 拉高。
    // 不再强制配置 PA14，避免沿用逐飞核心板/主板的板级假设。
    gpio_init(B22, GPO, GPIO_LOW, GPO_PUSH_PULL);
    gpio_init(B6,  GPO, GPIO_HIGH, GPO_PUSH_PULL);
}"""
    gpio_text, substitutions = pattern.subn(replacement, gpio_text, count=1)
    if substitutions != 1:
        raise RuntimeError(f"Cannot patch gpio_set_a0_a1_output in {gpio_file}")
    gpio_file.write_text(gpio_text, encoding="utf-8")

    syscfg = ti_config / "SeekFree_MSPM0G3507_Device_Library.syscfg"
    for old, new in (
        ('GPIO1.port                               = "PORTA";', 'GPIO1.port                               = "PORTB";'),
        ('GPIO1.$name                              = "LED_A14";', 'GPIO1.$name                              = "LED_PB22";'),
        ('GPIO1.associatedPins[0].$name            = "PIN_14";', 'GPIO1.associatedPins[0].$name            = "PIN_22";'),
        ('GPIO1.associatedPins[0].assignedPin      = "14";', 'GPIO1.associatedPins[0].assignedPin      = "22";'),
        ('GPIO1.associatedPins[0].pin.$assign      = "PA14";', 'GPIO1.associatedPins[0].pin.$assign      = "PB22";'),
    ):
        replace_required(syscfg, old, new)

    config_c = ti_config / "ti_msp_dl_config.c"
    for old, new in (
        ("LED_A14_PIN_14_IOMUX", "LED_PB22_PIN_22_IOMUX"),
        ("DL_GPIO_clearPins(GPIOA, LED_A14_PIN_14_PIN)", "DL_GPIO_clearPins(GPIOB, LED_PB22_PIN_22_PIN)"),
        ("DL_GPIO_enableOutput(GPIOA, LED_A14_PIN_14_PIN)", "DL_GPIO_enableOutput(GPIOB, LED_PB22_PIN_22_PIN)"),
    ):
        replace_required(config_c, old, new)

    config_h = ti_config / "ti_msp_dl_config.h"
    replacements = (
        ("Pin Group LED_A14", "Pin Group LED_PB22"),
        ("#define LED_A14_PORT                                                     (GPIOA)",
         "#define LED_PB22_PORT                                                    (GPIOB)"),
        ("Defines for PIN_14: GPIOA.14 with pinCMx 36 on package pin 7",
         "Defines for PIN_22: GPIOB.22 with pinCMx 50 on package pin 21"),
        ("#define LED_A14_PIN_14_PIN                                      (DL_GPIO_PIN_14)",
         "#define LED_PB22_PIN_22_PIN                                     (DL_GPIO_PIN_22)"),
        ("#define LED_A14_PIN_14_IOMUX                                     (IOMUX_PINCM36)",
         "#define LED_PB22_PIN_22_IOMUX                                    (IOMUX_PINCM50)"),
    )
    for old, new in replacements:
        replace_required(config_h, old, new)


def patch_demo_main(example_root: Path, demo: str, replacements: tuple[tuple[str, str], ...]) -> None:
    path = example_root / demo / "user" / "src" / "main.c"
    for old, new in replacements:
        replace_required(path, old, new)


def patch_examples(example_root: Path) -> None:
    patch_demo_main(example_root, "E01_gpio_demo", (
        ("GPIO A14", "GPIO B22"),
        ("gpio_init(A14,", "gpio_init(TMX_LED_PIN,"),
        ("gpio_set_level(A14,", "gpio_set_level(TMX_LED_PIN,"),
        ("gpio_toggle_level(A14)", "gpio_toggle_level(TMX_LED_PIN)"),
    ))
    patch_demo_main(example_root, "E04_pwm_demo", (
        ("PWM_TIM_A0_CH1_B9", "PWM_TIM_A0_CH1_B20"),
    ))
    patch_demo_main(example_root, "E05_pit_demo", (
        ("#define LED1                    (A14)", "#define LED1                    (TMX_LED_PIN)"),
    ))
    patch_demo_main(example_root, "E06_exti_demo", (
        ("#define LED1                    (A14 )", "#define LED1                    (TMX_LED_PIN)"),
        ("#define KEY1                    (A30)", "#define KEY1                    (TMX_KEY_PIN)"),
        ("#define KEY2                    (A31)", "#define KEY2                    (A30)"),
        ("#define KEY3                    (B0)", "#define KEY3                    (A31)"),
        ("#define KEY4                    (B1)", "#define KEY4                    (B0)"),
        ("exti_init(KEY1, EXTI_TRIGGER_RISING", "exti_init(KEY1, EXTI_TRIGGER_FALLING"),
    ))
    patch_demo_main(example_root, "E11_interrupt_priority_set_demo", (
        ("#define LED1                    (A14 )", "#define LED1                    (TMX_LED_PIN)"),
    ))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("upstream_root", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    upstream = args.upstream_root.resolve()
    library = upstream / "SeekFree_MSPM0G3507_Opensource_Library"
    core_demo = upstream / "Example" / "Coreboard_Demo"
    if not library.is_dir() or not core_demo.is_dir():
        raise FileNotFoundError("The selected directory is not the SeekFree MSPM0G3507 repository root")

    output = args.output.resolve()
    if output.exists():
        if not args.force:
            raise FileExistsError(f"Output exists: {output}; use --force to replace it")
        shutil.rmtree(output)

    output.mkdir(parents=True)
    output_library = output / "SeekFree_MSPM0G3507_Opensource_Library"
    output_examples = output / "Example" / "TianMengXing_Coreboard_Demo"
    shutil.copytree(library, output_library)
    shutil.copytree(core_demo, output_examples)

    overlay = Path(__file__).resolve().parents[1] / "overlay"
    copy_overlay(overlay, output)

    patch_common_library(output_library)
    patch_common_library(output_examples)
    patch_examples(output_examples)

    print(f"Created TianMengXing workspace: {output}")
    print("Open the Keil project and verify PB22 LED, UART0 PA10/PA11 and PB21 key on real hardware.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
