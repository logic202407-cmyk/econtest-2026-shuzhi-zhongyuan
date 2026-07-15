#!/usr/bin/env python3
"""Create a TianMengXing-adapted workspace from the upstream SeekFree tree.

Example:
    python materialize.py /path/to/MSPM0G3507_Library-master --output ./build/tianmengxing
"""
from __future__ import annotations

import argparse
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
    shutil.copytree(library, output / "SeekFree_MSPM0G3507_Opensource_Library")
    shutil.copytree(core_demo, output / "Example" / "TianMengXing_Coreboard_Demo")

    overlay = Path(__file__).resolve().parents[1] / "overlay"
    copy_overlay(overlay, output)

    print(f"Created TianMengXing workspace: {output}")
    print("Open the Keil project and verify PB22 LED, UART0 PA10/PA11 and PB21 key on real hardware.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
