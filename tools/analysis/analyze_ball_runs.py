"""Summarize H-balance serial CSV logs without machine-specific paths.

Usage:
    python tools/analysis/analyze_ball_runs.py run1.csv [run2.csv ...]

The input is expected to contain the telemetry columns emitted by the project:
log_ms, valid, target_cm, position_cm, error_to_target_cm, velocity_cm_s,
velocity_valid, frame_dt_ms and predicted.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def percentile(values: list[float], quantile: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    position = (len(ordered) - 1) * quantile
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    return ordered[lower] * (upper - position) + ordered[upper] * (position - lower)


def read_rows(path: Path) -> list[dict[str, float | int]]:
    rows: list[dict[str, float | int]] = []
    with path.open("r", encoding="utf-8-sig", errors="ignore", newline="") as handle:
        reader = csv.DictReader(line.replace("\x00", "") for line in handle)
        for row in reader:
            try:
                rows.append(
                    {
                        "t": int(row["log_ms"]),
                        "valid": int(row["valid"]),
                        "target": float(row["target_cm"]),
                        "pos": float(row["position_cm"]),
                        "err": float(row["error_to_target_cm"]),
                        "vel": float(row["velocity_cm_s"]),
                        "vvalid": int(row["velocity_valid"]),
                        "dt": int(row["frame_dt_ms"]),
                        "predicted": int(row["predicted"]),
                    }
                )
            except (KeyError, TypeError, ValueError):
                continue
    return rows


def outside_events(rows: list[dict[str, float | int]], band: float = 1.0,
                   minimum_ms: int = 500) -> list[dict[str, float | int]]:
    events: list[dict[str, float | int]] = []
    active: dict[str, float | int] | None = None
    for index, row in enumerate(rows):
        error = abs(float(row["err"]))
        if error > band and active is None:
            active = {"start": int(row["t"]), "peak_index": index, "peak": error}
        elif error > band and active is not None and error > float(active["peak"]):
            active.update({"peak_index": index, "peak": error})
        elif error <= band and active is not None:
            duration = int(row["t"]) - int(active["start"])
            if duration >= minimum_ms:
                active["duration_ms"] = duration
                events.append(active)
            active = None
    return events


def summarize(path: Path) -> dict[str, object]:
    rows = read_rows(path)
    if not rows:
        return {"file": path.name, "error": "no parseable telemetry rows"}
    valid = [row for row in rows if row["valid"]]
    if not valid:
        return {"file": path.name, "rows": len(rows), "valid_rows": 0}

    errors = [float(row["err"]) for row in valid]
    frame_intervals = [int(row["dt"]) for row in valid if int(row["dt"]) > 0]
    events = outside_events(valid)
    duration_ms = int(rows[-1]["t"]) - int(rows[0]["t"])
    return {
        "file": path.name,
        "duration_s": round(duration_ms / 1000, 3),
        "rows": len(rows),
        "valid_rows": len(valid),
        "valid_pct": round(100 * len(valid) / len(rows), 3),
        "targets_cm": sorted({float(row["target"]) for row in valid}),
        "mean_error_cm": round(statistics.fmean(errors), 4),
        "median_error_cm": round(statistics.median(errors), 4),
        "rmse_cm": round(math.sqrt(statistics.fmean(value * value for value in errors)), 4),
        "p95_abs_error_cm": round(percentile([abs(value) for value in errors], 0.95) or 0, 4),
        "within_1cm_pct": round(100 * sum(abs(value) <= 1 for value in errors) / len(errors), 3),
        "frame_dt_median_ms": statistics.median(frame_intervals) if frame_intervals else None,
        "frame_dt_p95_ms": percentile(frame_intervals, 0.95),
        "outside_1cm_events_ge_500ms": len(events),
        "largest_events": sorted(events, key=lambda event: float(event["peak"]), reverse=True)[:10],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", nargs="+", type=Path, help="telemetry CSV file")
    args = parser.parse_args()
    print(json.dumps([summarize(path) for path in args.csv], ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
