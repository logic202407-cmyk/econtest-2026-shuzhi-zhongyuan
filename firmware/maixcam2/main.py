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

from maix import err, pinmap, time, uart

try:
    from maix import camera, display, image
except Exception:
    camera = None
    display = None
    image = None


UART_DEVICE = "/dev/ttyS4"
BAUDRATE = 115200
TX_INTERVAL_MS = 50

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
HORIZONTAL_FOV_DEG = 70.0
VERTICAL_FOV_DEG = 43.0

# MaixPy find_blobs LAB threshold. This default is for a saturated red PCB.
# The A channel lower bound is intentionally high to reject skin color when a
# hand is holding the target. Tune in MaixVision/threshold editor if needed:
# [L_min, L_max, A_min, A_max, B_min, B_max]
COLOR_THRESHOLDS = [
    [10, 90, 35, 80, 0, 70],
]

MIN_PIXELS = 35
MIN_AREA = 35
MIN_BLOB_W = 4
MIN_BLOB_H = 4
MAX_BLOB_W = FRAME_WIDTH // 2
MAX_BLOB_H = FRAME_HEIGHT // 2
MAX_BLOB_AREA_X100 = 2200
MIN_BLOB_DENSITY_X100 = 14
MAX_ASPECT_RATIO_X100 = 450
LOST_CONFIRM_FRAMES = 3
LOST_HEARTBEAT_EVERY = 20

# Output stabilization. The camera runs at 20 Hz, so a small amount of
# filtering removes color-threshold jitter without making the gimbal feel dead.
TARGET_FILTER_ALPHA_X100 = 55
TARGET_OUTPUT_STEP_LIMIT_0P01DEG = 550
TARGET_SNAP_DEADBAND_0P01DEG = 18
TARGET_KEEP_CENTER_SCORE_PENALTY = 3

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


def blob_is_valid(blob):
    x, y, w, h = blob_rect(blob)
    pixels = blob_pixels(blob)
    area = max(1, w * h)

    if w < MIN_BLOB_W or h < MIN_BLOB_H:
        return False
    if w > MAX_BLOB_W or h > MAX_BLOB_H:
        return False
    if pixels < MIN_PIXELS or area < MIN_AREA:
        return False
    if (area * 100) // (FRAME_WIDTH * FRAME_HEIGHT) > MAX_BLOB_AREA_X100:
        return False
    if (pixels * 100) // area < MIN_BLOB_DENSITY_X100:
        return False

    larger = max(w, h)
    smaller = max(1, min(w, h))
    if (larger * 100) // smaller > MAX_ASPECT_RATIO_X100:
        return False

    return True


def select_target_blob(blobs, previous_rect):
    best_blob = None
    best_score = None

    if not blobs:
        return None

    previous_cx = None
    previous_cy = None
    if previous_rect is not None:
        px, py, pw, ph = previous_rect
        previous_cx = px + (pw // 2)
        previous_cy = py + (ph // 2)

    for blob in blobs:
        if not blob_is_valid(blob):
            continue

        x, y, w, h = blob_rect(blob)
        cx = x + (w // 2)
        cy = y + (h // 2)
        score = blob_pixels(blob) * 6 + (w * h)

        if previous_cx is not None:
            dx = cx - previous_cx
            dy = cy - previous_cy
            score -= (dx * dx + dy * dy) // TARGET_KEEP_CENTER_SCORE_PENALTY

        if best_score is None or score > best_score:
            best_score = score
            best_blob = blob

    return best_blob


def target_from_blob(blob):
    x, y, w, h = blob_rect(blob)
    cx = x + (w // 2)
    cy = y + (h // 2)

    yaw_deg = ((cx - (FRAME_WIDTH / 2.0)) / FRAME_WIDTH) * HORIZONTAL_FOV_DEG
    pitch_deg = (((FRAME_HEIGHT / 2.0) - cy) / FRAME_HEIGHT) * VERTICAL_FOV_DEG

    area_ratio = float(max(0, w * h)) / float(FRAME_WIDTH * FRAME_HEIGHT)
    confidence = 5600 + int(min(area_ratio * 90000.0, 3900.0))

    return int(yaw_deg * 100.0), int(pitch_deg * 100.0), confidence, (x, y, w, h)


def limit_step(previous, current, step_limit):
    delta = current - previous
    if delta > step_limit:
        return previous + step_limit
    if delta < -step_limit:
        return previous - step_limit
    return current


class TargetFilter:
    def __init__(self):
        self.valid = False
        self.yaw = 0
        self.pitch = 0
        self.confidence = 0
        self.rect = None
        self.missing_frames = 0

    def update_found(self, yaw, pitch, confidence, rect):
        if not self.valid:
            self.yaw = yaw
            self.pitch = pitch
            self.confidence = confidence
            self.valid = True
        else:
            yaw = limit_step(self.yaw, yaw, TARGET_OUTPUT_STEP_LIMIT_0P01DEG)
            pitch = limit_step(self.pitch, pitch, TARGET_OUTPUT_STEP_LIMIT_0P01DEG)

            self.yaw = (
                self.yaw * (100 - TARGET_FILTER_ALPHA_X100)
                + yaw * TARGET_FILTER_ALPHA_X100
            ) // 100
            self.pitch = (
                self.pitch * (100 - TARGET_FILTER_ALPHA_X100)
                + pitch * TARGET_FILTER_ALPHA_X100
            ) // 100
            self.confidence = (
                self.confidence * 40 + confidence * 60
            ) // 100

            if abs(self.yaw) < TARGET_SNAP_DEADBAND_0P01DEG:
                self.yaw = 0
            if abs(self.pitch) < TARGET_SNAP_DEADBAND_0P01DEG:
                self.pitch = 0

        self.rect = rect
        self.missing_frames = 0
        return self.yaw, self.pitch, self.confidence

    def update_missing(self):
        self.missing_frames += 1
        if self.valid and self.missing_frames < LOST_CONFIRM_FRAMES:
            confidence = max(0, self.confidence - self.missing_frames * 900)
            return True, self.yaw, self.pitch, confidence, self.rect

        self.valid = False
        self.rect = None
        self.confidence = 0
        return False, 0, 0, 0, None


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
    target_filter = TargetFilter()

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
            blob = select_target_blob(blobs, target_filter.rect)

            if blob is None:
                held, yaw, pitch, confidence, rect = target_filter.update_missing()
                if held:
                    frame = build_target_found_frame(yaw, pitch, confidence)
                    status = "TARGET HOLD"
                else:
                    frame = build_target_lost_frame()
                    lost_count += 1
                    rect = None
                    status = "TARGET LOST"
            else:
                yaw, pitch, confidence, rect = target_from_blob(blob)
                yaw, pitch, confidence = target_filter.update_found(
                    yaw, pitch, confidence, rect
                )
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
