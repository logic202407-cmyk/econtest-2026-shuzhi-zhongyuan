"""
MaixCAM Pro UART fake-data sender.

It periodically sends frames in the shared contest protocol:
    $V,mode,cx,cy,w,h,D,x,angle,conf#
"""

import time


BAUDRATE = 115200
TX_INTERVAL_MS = 100


def build_frame(mode, cx, cy, w, h, distance, x_offset, angle, confidence):
    return (
        f"$V,{mode},{cx},{cy},{w},{h},"
        f"{distance:.1f},{x_offset:.1f},{angle:.1f},{confidence:.2f}#"
    )


def fake_target(tick):
    mode = 1
    cx = 320 + (tick % 41) - 20
    cy = 240
    w = 80
    h = 80
    distance = 150.0
    x_offset = (cx - 320) / 2.5
    angle = x_offset * 0.8
    confidence = 0.99
    return mode, cx, cy, w, h, distance, x_offset, angle, confidence


def open_uart():
    try:
        from machine import UART

        return UART(1, BAUDRATE)
    except Exception:
        return None


def main():
    uart = open_uart()
    tick = 0

    while True:
        frame = build_frame(*fake_target(tick))
        data = frame + "\r\n"

        if uart is not None:
            uart.write(data)
        else:
            print(data, end="")

        tick += 1
        time.sleep_ms(TX_INTERVAL_MS) if hasattr(time, "sleep_ms") else time.sleep(TX_INTERVAL_MS / 1000)


if __name__ == "__main__":
    main()

