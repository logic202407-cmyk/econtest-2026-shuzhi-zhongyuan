"""Monte-Carlo plant check for the H-problem ball-balance controller.

This is a design-time model, not evidence of real-hardware performance. It
varies vision delay, motor time constant, rolling acceleration, measurement
noise, and starting position while using the same control constants as the C
firmware.
"""

from __future__ import annotations

import argparse
import math
import random
import statistics


CONTROL_PERIOD_S = 0.025
SIMULATION_STEP_S = 0.005
SIMULATION_LIMIT_S = 15.0
TARGET_CM = 12.5
PIPE_LENGTH_CM = 25.0
EDGE_GUARD_CM = 1.0
MAX_TILT_DEG = 5.0
TILT_SLEW_DEG = 0.5
PROFILE_MAX_VEL_CM_S = 16.0
PROFILE_BRAKE_ACC_CM_S2 = 18.0
PROFILE_POS_TO_VEL = 1.3
PROFILE_VEL_KP = 5.0
INTEGRAL_BAND_CM = 3.0
INTEGRAL_KI = 0.8
INTEGRAL_ACC_MAX_CM_S2 = 5.0
MAX_ACC_CM_S2 = 61.0
NOMINAL_ROLLING_ACC_CM_S2_PER_RAD = (5.0 / 7.0) * 981.0


def clamp(value: float, minimum: float, maximum: float) -> float:
    return max(minimum, min(maximum, value))


def controller(target: float, position: float, velocity: float,
               integral: float) -> float:
    if position <= EDGE_GUARD_CM:
        return MAX_TILT_DEG
    if position >= PIPE_LENGTH_CM - EDGE_GUARD_CM:
        return -MAX_TILT_DEG

    error = target - position
    reference_speed = min(
        PROFILE_MAX_VEL_CM_S,
        math.sqrt(2.0 * PROFILE_BRAKE_ACC_CM_S2 * abs(error)),
        PROFILE_POS_TO_VEL * abs(error),
    )
    if error < 0.0:
        reference_speed = -reference_speed
    integral_acc = clamp(
        INTEGRAL_KI * integral,
        -INTEGRAL_ACC_MAX_CM_S2,
        INTEGRAL_ACC_MAX_CM_S2,
    )
    acceleration = clamp(
        PROFILE_VEL_KP * (reference_speed - velocity) + integral_acc,
        -MAX_ACC_CM_S2,
        MAX_ACC_CM_S2,
    )
    return clamp(acceleration / 12.2, -MAX_TILT_DEG, MAX_TILT_DEG)


def run_case(seed: int) -> float | None:
    rng = random.Random(seed)
    position = rng.choice((0.0, 2.0, 5.0, 20.0, 23.0, 25.0))
    velocity = 0.0
    requested_tilt = 0.0
    motor_tilt = 0.0
    integral = 0.0
    settle_time = 0.0
    next_control = 0.0
    history: list[tuple[float, float, float]] = []

    vision_delay = rng.uniform(0.030, 0.100)
    motor_time_constant = rng.uniform(0.040, 0.140)
    plant_scale = rng.uniform(0.65, 1.20)

    steps = int(SIMULATION_LIMIT_S / SIMULATION_STEP_S)
    for step in range(steps):
        now = step * SIMULATION_STEP_S
        history.append((now, position, velocity))
        while history and history[0][0] < now - vision_delay - 0.010:
            history.pop(0)
        delayed = min(history, key=lambda sample: abs(
            sample[0] - (now - vision_delay)
        ))

        if now + 1e-9 >= next_control:
            next_control += CONTROL_PERIOD_S
            measured_velocity = delayed[2] + rng.gauss(0.0, 0.6)
            # Firmware uses processing_ms and measured velocity to compensate
            # the capture-to-control delay before its alpha-beta update.
            measured_position = (
                delayed[1]
                + measured_velocity * vision_delay
                + rng.gauss(0.0, 0.05)
            )
            error = TARGET_CM - measured_position
            if abs(error) <= INTEGRAL_BAND_CM:
                integral = clamp(
                    integral + error * CONTROL_PERIOD_S, -50.0, 50.0
                )
            else:
                integral *= 0.9
            desired_tilt = controller(
                TARGET_CM, measured_position, measured_velocity, integral
            )
            requested_tilt += clamp(
                desired_tilt - requested_tilt,
                -TILT_SLEW_DEG,
                TILT_SLEW_DEG,
            )

        motor_tilt += (
            (requested_tilt - motor_tilt)
            * min(1.0, SIMULATION_STEP_S / motor_time_constant)
        )
        acceleration = (
            plant_scale
            * NOMINAL_ROLLING_ACC_CM_S2_PER_RAD
            * math.sin(math.radians(motor_tilt))
            - 0.18 * velocity
        )
        if abs(velocity) > 0.02:
            acceleration -= math.copysign(0.3, velocity)
        velocity += acceleration * SIMULATION_STEP_S
        position += velocity * SIMULATION_STEP_S

        if position < 0.0 or position > PIPE_LENGTH_CM:
            return None
        if abs(position - TARGET_CM) <= 0.5 and abs(velocity) <= 1.0:
            settle_time += SIMULATION_STEP_S
        else:
            settle_time = 0.0
        if settle_time >= 0.5:
            return now - 0.5

    return None


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", type=int, default=500)
    parser.add_argument("--seed", type=int, default=0)
    args = parser.parse_args()

    results = [run_case(args.seed + index) for index in range(args.cases)]
    passed = sorted(value for value in results if value is not None)
    failed = len(results) - len(passed)
    if not passed:
        raise SystemExit("all simulation cases failed")
    p95_index = min(len(passed) - 1, int(len(passed) * 0.95))
    print("cases={}, passed={}, failed={}".format(args.cases, len(passed), failed))
    print(
        "settle_s median={:.3f}, p95={:.3f}, max={:.3f}".format(
            statistics.median(passed), passed[p95_index], max(passed)
        )
    )
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
