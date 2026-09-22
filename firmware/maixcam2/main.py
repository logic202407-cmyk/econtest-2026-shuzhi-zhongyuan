"""
MaixCAM2 first real-vision sender for the contest vision link.

Hardware:
    MaixCAM2 A21 / UART4_TX -> TianMengXing A9 / UART1_RX
    MaixCAM2 A22 / UART4_RX -> TianMengXing A8 / UART1_TX, optional
    MaixCAM2 GND            -> TianMengXing GND

Default behavior:
    Detect the largest red color blob, estimate yaw/pitch error from the image
    center, and send the project's binary TARGET_FOUND frame at 20 Hz.

Protocol:
    AA 55 CMD LEN DATA CHECK
    TARGET_FOUND DATA = yaw:int16_le, pitch:int16_le, confidence:uint16_le
    yaw/pitch unit = 0.01 degree, confidence unit = 0.01 percent

Temporary vision sign convention:
    yaw   > 0 means the target is on the right side of the image.
    pitch > 0 means the target is above the image center.
"""

import math

from maix import err, pinmap, time, uart

try:
    from maix import camera, display, image
except Exception:
    camera = None
    display = None
    image = None


UART_DEVICE = "/dev/ttyS4"
BAUDRATE = 115200
TX_INTERVAL_MS = 40

# Run modes:
#   "color" sends real color-blob vision data.
#   "fixed" sends the known-good frame verified with TianMengXing.
#   "fake"  sends a small sweeping fake target.
RUN_MODE = "color"
SHOW_PREVIEW = True

# Use 640x480 for the real gimbal test. 320x240 is faster, but the far target
# becomes too small and color blobs are easier to merge with the hand.
FRAME_WIDTH = 640
FRAME_HEIGHT = 480

# Rough first-test camera field of view. These only convert pixel error into a
# readable angle estimate; tune them later with real target geometry.
HORIZONTAL_FOV_DEG = 90.0
VERTICAL_FOV_DEG = 51.0

# MaixPy find_blobs LAB threshold. This default is for a saturated red target.
# Red has a positive A component, which separates it from ordinary skin tones
# under normal indoor lighting. Tune this one six-value threshold in MaixVision
# if the actual target or lighting differs:
# [L_min, L_max, A_min, A_max, B_min, B_max]
COLOR_THRESHOLDS = [
    [10, 90, 35, 80, 0, 70],
]

MIN_PIXELS = 18
MIN_AREA = 18
LOST_HEARTBEAT_EVERY = 20

# yaw = 1.23 deg, pitch = -0.45 deg, confidence = 98.50%.
# This exact frame is also used in docs/maixcam_protocol.md and host tests.
KNOWN_GOOD_TARGET_FRAME = bytes(
    (0xAA, 0x55, 0x01, 0x06, 0x7B, 0x00, 0xD3, 0xFF, 0x7A, 0x26, 0xF4)
)


def build_target_found_frame(yaw_0p01deg, pitch_0p01deg, confidence_0p01pct):
    yaw_0p01deg = clamp_int(yaw_0p01deg, -32768, 32767)
    pitch_0p01deg = clamp_int(pitch_0p01deg, -32768, 32767)
    confidence_0p01pct = clamp_int(confidence_0p01pct, 0, 10000)

    payload = bytearray(6)
    payload[0:2] = int(yaw_0p01deg).to_bytes(2, "little", signed=True)
    payload[2:4] = int(pitch_0p01deg).to_bytes(2, "little", signed=True)
    payload[4:6] = int(confidence_0p01pct).to_bytes(2, "little", signed=False)

    command = 0x01
    frame = bytearray((0xAA, 0x55, command, len(payload)))
    frame.extend(payload)
    frame.append(sum(frame[2:]) & 0xFF)
    return bytes(frame)


def build_target_lost_frame():
    command = 0x02
    frame = bytearray((0xAA, 0x55, command, 0x00))
    frame.append(sum(frame[2:]) & 0xFF)
    return bytes(frame)


def fake_target(tick):
    # Sweep yaw from -1.00 to +1.00 degrees. Gimbal motion is disabled in the
    # MSPM0 configuration during communications bring-up, so this is safe.
    yaw_0p01deg = (tick % 201) - 100
    pitch_0p01deg = 0
    confidence_0p01pct = 9900
    return yaw_0p01deg, pitch_0p01deg, confidence_0p01pct


def clamp_int(value, minimum, maximum):
    value = int(value)
    if value < minimum:
        return minimum
    if value > maximum:
        return maximum
    return value


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


def blob_value(blob, index, name):
    if hasattr(blob, name):
        value = getattr(blob, name)
        return value() if callable(value) else value
    return blob[index]


def blob_rect(blob):
    x = int(blob_value(blob, 0, "x"))
    y = int(blob_value(blob, 1, "y"))
    w = int(blob_value(blob, 2, "w"))
    h = int(blob_value(blob, 3, "h"))
    return x, y, w, h


def blob_pixels(blob):
    if hasattr(blob, "pixels"):
        value = getattr(blob, "pixels")
        return int(value() if callable(value) else value)
    x, y, w, h = blob_rect(blob)
    return int(w * h)


def largest_blob(blobs):
    if not blobs:
        return None
    return max(blobs, key=blob_pixels)


def target_from_blob(blob):
    x, y, w, h = blob_rect(blob)
    cx = x + (w // 2)
    cy = y + (h // 2)

    # Perspective projection is important at the edge of a wide-angle lens.
    # The previous linear conversion understated a far-off-center target,
    # which made the yaw axis react too weakly for distant objects.
    focal_x = (FRAME_WIDTH / 2.0) / math.tan(math.radians(HORIZONTAL_FOV_DEG / 2.0))
    focal_y = (FRAME_HEIGHT / 2.0) / math.tan(math.radians(VERTICAL_FOV_DEG / 2.0))
    yaw_deg = math.degrees(math.atan((cx - (FRAME_WIDTH / 2.0)) / focal_x))
    pitch_deg = math.degrees(math.atan(((FRAME_HEIGHT / 2.0) - cy) / focal_y))

    # A blob that passes the calibrated red threshold is either the target or
    # not a target. Its apparent area must not make distant valid targets lose
    # tracking, so confidence stays fixed.
    return int(yaw_deg * 100.0), int(pitch_deg * 100.0), 9900, (x, y, w, h)


def open_camera():
    if camera is None:
        raise RuntimeError("MaixPy camera module is not available")
    return camera.Camera(FRAME_WIDTH, FRAME_HEIGHT)


def open_display():
    if not SHOW_PREVIEW or display is None:
        return None
    try:
        return display.Display()
    except Exception:
        return None


def draw_preview(img, rect, status):
    if image is None:
        return

    try:
        color_green = image.COLOR_GREEN
        color_red = image.COLOR_RED
        color_white = image.COLOR_WHITE
    except Exception:
        color_green = (0, 255, 0)
        color_red = (255, 0, 0)
        color_white = (255, 255, 255)

    try:
        img.draw_cross(FRAME_WIDTH // 2, FRAME_HEIGHT // 2, color=color_white)
        if rect is not None:
            x, y, w, h = rect
            img.draw_rect(x, y, w, h, color=color_green, thickness=2)
            img.draw_cross(x + w // 2, y + h // 2, color=color_green)
        else:
            img.draw_string(4, 4, status, color=color_red)
    except Exception:
        pass


def main():
    serial = open_uart4()
    tick = 0
    lost_count = 0

    if RUN_MODE == "color":
        cam = open_camera()
        disp = open_display()
    else:
        cam = None
        disp = None

    while True:
        if RUN_MODE == "fixed":
            frame = KNOWN_GOOD_TARGET_FRAME
        elif RUN_MODE == "fake":
            frame = build_target_found_frame(*fake_target(tick))
        else:
            img = cam.read()
            blobs = img.find_blobs(
                COLOR_THRESHOLDS,
                pixels_threshold=MIN_PIXELS,
                area_threshold=MIN_AREA,
                merge=False,
            )
            blob = largest_blob(blobs)

            if blob is None:
                frame = build_target_lost_frame()
                lost_count += 1
                rect = None
                status = "TARGET LOST"
            else:
                yaw, pitch, confidence, rect = target_from_blob(blob)
                frame = build_target_found_frame(yaw, pitch, confidence)
                lost_count = 0
                status = "TARGET FOUND"

            if disp is not None:
                draw_preview(img, rect, status)
                disp.show(img)

        serial.write(frame)
        tick += 1

        # Send lost frames continuously, but leave a tiny hint for future
        # console-side debugging without flooding UART0.
        if RUN_MODE == "color" and lost_count > 0:
            lost_count %= LOST_HEARTBEAT_EVERY

        time.sleep_ms(TX_INTERVAL_MS)


if __name__ == "__main__":
    main()
