#!/usr/bin/env python3
"""Generate and optionally transmit MaixCAM and X42S protocol frames."""

from __future__ import annotations

import argparse
import struct
import sys
import time
from typing import Iterable


MAIX_HEADER = b"\xAA\x55"
MAIX_TARGET_FOUND = 0x01
MAIX_TARGET_LOST = 0x02
MAIX_HEARTBEAT = 0x03
MAIX_ERROR = 0x04
X42S_CHECK = 0x6B


def maix_frame(command: int, payload: bytes = b"") -> bytes:
    if len(payload) > 16:
        raise ValueError("MaixCAM payload must be 16 bytes or less")
    body = bytes((command, len(payload))) + payload
    return MAIX_HEADER + body + bytes((sum(body) & 0xFF,))


def maix_target(yaw_deg: float, pitch_deg: float, confidence_pct: float) -> bytes:
    yaw = to_signed_units(yaw_deg, 100, "yaw")
    pitch = to_signed_units(pitch_deg, 100, "pitch")
    confidence = to_unsigned_units(confidence_pct, 100, "confidence", 10000)
    return maix_frame(MAIX_TARGET_FOUND, struct.pack("<hhH", yaw, pitch, confidence))


def maix_heartbeat(uptime_ms: int, sequence: int) -> bytes:
    if not 0 <= uptime_ms <= 0xFFFFFFFF:
        raise ValueError("uptime must fit in uint32")
    if not 0 <= sequence <= 0xFFFF:
        raise ValueError("sequence must fit in uint16")
    return maix_frame(MAIX_HEARTBEAT, struct.pack("<IH", uptime_ms, sequence))


def x42s_enable(motor_id: int, enabled: bool) -> bytes:
    return bytes((motor_id, 0xF3, 0xAB, int(enabled), 0x00, X42S_CHECK))


def x42s_stop(motor_id: int) -> bytes:
    return bytes((motor_id, 0xFE, 0x98, 0x00, X42S_CHECK))


def x42s_position(
    motor_id: int,
    position_0p1deg: int,
    acceleration_rpm_s: int,
    deceleration_rpm_s: int,
    speed_0p1rpm: int,
    raf: int,
    sync: bool,
) -> bytes:
    validate_motor_id(motor_id)
    validate_u16(acceleration_rpm_s, "acceleration")
    validate_u16(deceleration_rpm_s, "deceleration")
    validate_u16(speed_0p1rpm, "speed")
    if not 0 <= raf <= 0xFF:
        raise ValueError("raf must fit in uint8")
    magnitude = abs_i32(position_0p1deg, "position")
    direction = 1 if position_0p1deg < 0 else 0
    return (
        bytes((motor_id, 0xFD, direction))
        + struct.pack(">HHHI", acceleration_rpm_s, deceleration_rpm_s, speed_0p1rpm, magnitude)
        + bytes((raf, int(sync), X42S_CHECK))
    )


def x42s_speed(
    motor_id: int, speed_0p1rpm: int, acceleration_rpm_s: int, sync: bool
) -> bytes:
    validate_motor_id(motor_id)
    validate_u16(acceleration_rpm_s, "acceleration")
    magnitude = abs_i32(speed_0p1rpm, "speed")
    validate_u16(magnitude, "speed")
    direction = 1 if speed_0p1rpm < 0 else 0
    return bytes((motor_id, 0xF6, direction)) + struct.pack(">HH", acceleration_rpm_s, magnitude) + bytes((int(sync), X42S_CHECK))


def x42s_request(motor_id: int, command: int) -> bytes:
    validate_motor_id(motor_id)
    return bytes((motor_id, command, X42S_CHECK))


def validate_motor_id(motor_id: int) -> None:
    if not 0 <= motor_id <= 0xFF:
        raise ValueError("motor id must fit in uint8")


def validate_u16(value: int, name: str) -> None:
    if not 0 <= value <= 0xFFFF:
        raise ValueError(f"{name} must fit in uint16")


def abs_i32(value: int, name: str) -> int:
    if not -(1 << 31) <= value <= (1 << 31) - 1:
        raise ValueError(f"{name} must fit in int32")
    return abs(value)


def to_signed_units(value: float, scale: int, name: str) -> int:
    units = round(value * scale)
    if not -(1 << 15) <= units <= (1 << 15) - 1:
        raise ValueError(f"{name} is outside signed int16 range")
    return units


def to_unsigned_units(value: float, scale: int, name: str, maximum: int) -> int:
    units = round(value * scale)
    if not 0 <= units <= maximum:
        raise ValueError(f"{name} must be between 0 and {maximum / scale:g}")
    return units


def transmit(frame: bytes, port: str | None, baudrate: int, repeat: int, interval_s: float) -> None:
    if repeat < 1:
        raise ValueError("repeat must be at least one")
    if interval_s < 0:
        raise ValueError("interval must not be negative")

    print(frame.hex(" ").upper())
    if port is None:
        return

    try:
        import serial  # type: ignore[import-not-found]
    except ImportError as exc:
        raise RuntimeError("Install pyserial first: pip install pyserial") from exc

    with serial.Serial(port=port, baudrate=baudrate, bytesize=8, parity="N", stopbits=1, timeout=0.2) as uart:
        for index in range(repeat):
            uart.write(frame)
            uart.flush()
            if index + 1 < repeat:
                time.sleep(interval_s)


def add_transport_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--port", help="Serial port to transmit to, for example COM7")
    parser.add_argument("--baud", type=int, default=115200, help="UART baud rate (default: 115200)")
    parser.add_argument("--repeat", type=int, default=1, help="Number of sends (default: 1)")
    parser.add_argument("--interval", type=float, default=0.05, help="Seconds between sends (default: 0.05)")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    protocol = parser.add_subparsers(dest="protocol", required=True)

    maix = protocol.add_parser("maixcam", help="MaixCAM binary protocol")
    maix_commands = maix.add_subparsers(dest="command", required=True)
    target = maix_commands.add_parser("target", help="Send target-found frame")
    target.add_argument("--yaw", type=float, required=True, help="Yaw in degrees")
    target.add_argument("--pitch", type=float, required=True, help="Pitch in degrees")
    target.add_argument("--confidence", type=float, required=True, help="Confidence in percent")
    add_transport_options(target)
    lost = maix_commands.add_parser("lost", help="Send target-lost frame")
    add_transport_options(lost)
    heartbeat = maix_commands.add_parser("heartbeat", help="Send heartbeat frame")
    heartbeat.add_argument("--uptime", type=int, required=True, help="Uptime in milliseconds")
    heartbeat.add_argument("--sequence", type=int, required=True, help="Sequence number")
    add_transport_options(heartbeat)
    error = maix_commands.add_parser("error", help="Send remote-error frame")
    error.add_argument("--code", type=lambda value: int(value, 0), required=True, help="Error code byte")
    add_transport_options(error)

    x42s = protocol.add_parser("x42s", help="X42S X-firmware free protocol")
    x42s_commands = x42s.add_subparsers(dest="command", required=True)
    for name, help_text in (("enable", "Enable motor"), ("disable", "Disable motor"), ("stop", "Stop motor"), ("read-position", "Request position"), ("read-speed", "Request speed")):
        command = x42s_commands.add_parser(name, help=help_text)
        command.add_argument("--id", type=int, required=True, help="Motor ID")
        add_transport_options(command)
    position = x42s_commands.add_parser("position", help="Set position in 0.1 degree units")
    position.add_argument("--id", type=int, required=True, help="Motor ID")
    position.add_argument("--position", type=int, required=True, help="Position in 0.1 degree units")
    position.add_argument("--acc", type=int, default=100, help="Acceleration in RPM/s")
    position.add_argument("--dec", type=int, default=100, help="Deceleration in RPM/s")
    position.add_argument("--speed", type=int, default=300, help="Speed in 0.1 RPM units")
    position.add_argument("--raf", type=int, default=2, help="Relative/absolute flag value")
    position.add_argument("--sync", action="store_true", help="Set synchronous execution flag")
    add_transport_options(position)
    speed = x42s_commands.add_parser("speed", help="Set speed in 0.1 RPM units")
    speed.add_argument("--id", type=int, required=True, help="Motor ID")
    speed.add_argument("--speed", type=int, required=True, help="Signed speed in 0.1 RPM units")
    speed.add_argument("--acc", type=int, default=100, help="Acceleration in RPM/s")
    speed.add_argument("--sync", action="store_true", help="Set synchronous execution flag")
    add_transport_options(speed)
    return parser


def make_frame(args: argparse.Namespace) -> bytes:
    if args.protocol == "maixcam":
        if args.command == "target":
            return maix_target(args.yaw, args.pitch, args.confidence)
        if args.command == "lost":
            return maix_frame(MAIX_TARGET_LOST)
        if args.command == "heartbeat":
            return maix_heartbeat(args.uptime, args.sequence)
        if args.command == "error":
            if not 0 <= args.code <= 0xFF:
                raise ValueError("error code must fit in uint8")
            return maix_frame(MAIX_ERROR, bytes((args.code,)))

    if args.command == "enable":
        return x42s_enable(args.id, True)
    if args.command == "disable":
        return x42s_enable(args.id, False)
    if args.command == "stop":
        return x42s_stop(args.id)
    if args.command == "read-position":
        return x42s_request(args.id, 0x0F)
    if args.command == "read-speed":
        return x42s_request(args.id, 0x0E)
    if args.command == "position":
        return x42s_position(args.id, args.position, args.acc, args.dec, args.speed, args.raf, args.sync)
    if args.command == "speed":
        return x42s_speed(args.id, args.speed, args.acc, args.sync)
    raise ValueError("Unsupported command")


def main(argv: Iterable[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        transmit(make_frame(args), args.port, args.baud, args.repeat, args.interval)
    except (RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
