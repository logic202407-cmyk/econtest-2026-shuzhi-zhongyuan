"""
MaixCAM2 UART fake-data sender for the contest vision link.

Hardware:
    MaixCAM2 A21 / UART4_TX -> TianMengXing A9 / UART1_RX
    MaixCAM2 A22 / UART4_RX -> TianMengXing A8 / UART1_TX
    MaixCAM2 GND            -> TianMengXing GND

Protocol:
    This script sends the project's formal binary target frame:
        AA 55 01 06 yaw_le pitch_le confidence_le check

    yaw and pitch use signed 0.01 degree units. confidence uses unsigned
    0.01 percent units. This is the same protocol parsed by the TianMengXing
    MSPM0 application, so a valid link prints VISION,FRAME on UART0.
"""

from maix import err, pinmap, time, uart


UART_DEVICE = "/dev/ttyS4"
BAUDRATE = 115200
TX_INTERVAL_MS = 50
FIXED_TEST_FRAME = True

# yaw = 1.23 deg, pitch = -0.45 deg, confidence = 98.50%.
# This exact frame is also used in docs/maixcam_protocol.md and host tests.
KNOWN_GOOD_TARGET_FRAME = bytes(
    (0xAA, 0x55, 0x01, 0x06, 0x7B, 0x00, 0xD3, 0xFF, 0x7A, 0x26, 0xF4)
)


def build_target_found_frame(yaw_0p01deg, pitch_0p01deg, confidence_0p01pct):
    payload = bytearray(6)
    payload[0:2] = int(yaw_0p01deg).to_bytes(2, "little", signed=True)
    payload[2:4] = int(pitch_0p01deg).to_bytes(2, "little", signed=True)
    payload[4:6] = int(confidence_0p01pct).to_bytes(2, "little", signed=False)

    command = 0x01
    frame = bytearray((0xAA, 0x55, command, len(payload)))
    frame.extend(payload)
    frame.append(sum(frame[2:]) & 0xFF)
    return bytes(frame)


def fake_target(tick):
    # Sweep yaw from -1.00 to +1.00 degrees. Gimbal motion is disabled in the
    # MSPM0 configuration during communications bring-up, so this is safe.
    yaw_0p01deg = (tick % 201) - 100
    pitch_0p01deg = 0
    confidence_0p01pct = 9900
    return yaw_0p01deg, pitch_0p01deg, confidence_0p01pct


def open_uart4():
    err.check_raise(
        pinmap.set_pin_function("A21", "UART4_TX"),
        "set MaixCAM2 A21 to UART4_TX failed",
    )
    err.check_raise(
        pinmap.set_pin_function("A22", "UART4_RX"),
        "set MaixCAM2 A22 to UART4_RX failed",
    )
    return uart.UART(UART_DEVICE, BAUDRATE)


def main():
    serial = open_uart4()
    tick = 0

    while True:
        if FIXED_TEST_FRAME:
            frame = KNOWN_GOOD_TARGET_FRAME
        else:
            frame = build_target_found_frame(*fake_target(tick))
        serial.write(frame)
        tick += 1
        time.sleep_ms(TX_INTERVAL_MS)


if __name__ == "__main__":
    main()
