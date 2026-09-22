"""Low-latency MaixCAM2 vision front end for the H-problem ball balancer.

Coordinate convention:
    0 cm is the X42S lift end of the green pipe (image-right by default), and
    position increases toward the fixed end. The MSPM0 controller therefore
    maps positive ball acceleration directly to CW / increasing motor angle.

UART frame:
    AA 55 10 10 PAYLOAD CHECK
    PAYLOAD = seq:u16, capture_ms:u32, position_0p01cm:i16,
              velocity_0p01cm_s:i16, confidence_0p01pct:u16,
              processing_ms:u16, flags:u8, source:u8
"""

import math

import cv2
import numpy as np
from maix import app, camera, display, err, image, nn, pinmap, time, uart


MODEL_PATH = "/root/mymodels/demo03_test/best.mud"
BALL_CLASS_ID = 0
BALL_CONFIDENCE = 0.28
BALL_IOU = 0.45

PIPE_LENGTH_CM = 25.0
LIFT_END_IS_IMAGE_RIGHT = True
GREEN_HSV_LOW = (35, 50, 35)
GREEN_HSV_HIGH = (92, 255, 255)
MIN_PIPE_AREA = 700.0
MIN_PIPE_ASPECT = 5.0
MIN_PIPE_LENGTH_PX = 220.0
MAX_PIPE_WIDTH_PX = 90.0
PIPE_LOCK_FRAMES = 8
PIPE_LOCK_MAX_JUMP_PX = 45.0
PIPE_MISSING_RESET_FRAMES = 20
PIPE_TRACK_ALPHA = 0.20

BALL_AXIS_MARGIN_PX = 24.0
BALL_END_MARGIN = 0.08
BALL_MAX_SPEED_CM_S = 65.0
BALL_JUMP_ALLOWANCE_CM = 0.45
VELOCITY_WINDOW_MS = 130
VELOCITY_MIN_SPAN_MS = 45
VELOCITY_MIN_SAMPLES = 3
VELOCITY_ALPHA = 0.45

TARGET_POSITION_CM = 12.5
UART_DEVICE = "/dev/ttyS4"
UART_BAUDRATE = 115200
UART_MIN_SEND_MS = 25
TARGET_REFRESH_MS = 1000
SHOW_PREVIEW = True

FRAME_HEADER_0 = 0xAA
FRAME_HEADER_1 = 0x55
CMD_BALL_STATE = 0x10
CMD_TARGET_SELECT = 0x11
FLAG_VALID = 0x01
FLAG_VELOCITY_VALID = 0x02
FLAG_PIPE_LOCKED = 0x04
SOURCE_LOST = 0
SOURCE_YOLO = 1

OPEN_KERNEL = np.ones((3, 3), np.uint8)
CLOSE_KERNEL = np.ones((5, 17), np.uint8)


def clamp(value, minimum, maximum):
    if value < minimum:
        return minimum
    if value > maximum:
        return maximum
    return value


def elapsed_ms(now_ms, then_ms):
    return (int(now_ms) - int(then_ms)) & 0xFFFFFFFF


def build_frame(command, payload):
    frame = bytearray((FRAME_HEADER_0, FRAME_HEADER_1, command, len(payload)))
    frame.extend(payload)
    frame.append(sum(frame[2:]) & 0xFF)
    return bytes(frame)


def build_target_frame(target_cm):
    value = int(round(clamp(target_cm, 0.0, PIPE_LENGTH_CM) * 100.0))
    return build_frame(
        CMD_TARGET_SELECT,
        int(value).to_bytes(2, "little", signed=True),
    )


def build_ball_frame(
    seq,
    capture_ms,
    position_cm,
    velocity_cm_s,
    confidence,
    processing_ms,
    valid,
    velocity_valid,
    pipe_locked,
):
    flags = 0
    if valid:
        flags |= FLAG_VALID
    if velocity_valid:
        flags |= FLAG_VELOCITY_VALID
    if pipe_locked:
        flags |= FLAG_PIPE_LOCKED

    payload = bytearray()
    payload.extend((int(seq) & 0xFFFF).to_bytes(2, "little"))
    payload.extend((int(capture_ms) & 0xFFFFFFFF).to_bytes(4, "little"))
    payload.extend(
        int(clamp(round(position_cm * 100.0), -32768, 32767)).to_bytes(
            2, "little", signed=True
        )
    )
    payload.extend(
        int(clamp(round(velocity_cm_s * 100.0), -32768, 32767)).to_bytes(
            2, "little", signed=True
        )
    )
    payload.extend(
        int(clamp(round(confidence * 10000.0), 0, 10000)).to_bytes(
            2, "little"
        )
    )
    payload.extend(
        int(clamp(processing_ms, 0, 65535)).to_bytes(2, "little")
    )
    payload.append(flags)
    payload.append(SOURCE_YOLO if valid else SOURCE_LOST)
    if len(payload) != 16:
        raise RuntimeError("BALL_STATE payload must remain 16 bytes")
    return build_frame(CMD_BALL_STATE, payload)


def open_uart4():
    err.check_raise(
        pinmap.set_pin_function("A21", "UART4_TX"),
        "set A21 to UART4_TX failed",
    )
    err.check_raise(
        pinmap.set_pin_function("A22", "UART4_RX"),
        "set A22 to UART4_RX failed",
    )
    return uart.UART(UART_DEVICE, UART_BAUDRATE)


class PipeTracker:
    """Detect and track the pipe as a directed lift-to-fixed axis."""

    def __init__(self):
        self.samples = []
        self.axis = None
        self.locked = False
        self.missing = 0

    @staticmethod
    def _detect(frame):
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        mask = cv2.inRange(hsv, GREEN_HSV_LOW, GREEN_HSV_HIGH)
        blue, green, red = cv2.split(frame)
        dominance = cv2.subtract(green, cv2.max(red, blue))
        _, dominance = cv2.threshold(dominance, 10, 255, cv2.THRESH_BINARY)
        mask = cv2.bitwise_and(mask, dominance)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, OPEN_KERNEL)
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, CLOSE_KERNEL)
        contours, _ = cv2.findContours(
            mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )

        best = None
        best_score = 0.0
        for contour in contours:
            area = float(cv2.contourArea(contour))
            if area < MIN_PIPE_AREA:
                continue
            (cx, cy), (width, height), angle = cv2.minAreaRect(contour)
            long_side = max(float(width), float(height))
            short_side = min(float(width), float(height))
            if short_side < 2.0:
                continue
            aspect = long_side / short_side
            if (
                aspect < MIN_PIPE_ASPECT
                or long_side < MIN_PIPE_LENGTH_PX
                or short_side > MAX_PIPE_WIDTH_PX
            ):
                continue

            if width < height:
                angle += 90.0
            radians = math.radians(float(angle))
            dx = math.cos(radians)
            dy = math.sin(radians)
            x0 = float(cx) - dx * long_side * 0.5
            y0 = float(cy) - dy * long_side * 0.5
            x1 = float(cx) + dx * long_side * 0.5
            y1 = float(cy) + dy * long_side * 0.5
            if LIFT_END_IS_IMAGE_RIGHT:
                lift, fixed = ((x1, y1), (x0, y0)) if x1 >= x0 else ((x0, y0), (x1, y1))
            else:
                lift, fixed = ((x0, y0), (x1, y1)) if x1 >= x0 else ((x1, y1), (x0, y0))

            fill = area / max(1.0, long_side * short_side)
            score = area * fill * min(aspect, 20.0)
            if score > best_score:
                best_score = score
                best = (
                    lift[0], lift[1], fixed[0], fixed[1],
                    short_side * 0.5, clamp(fill, 0.0, 1.0),
                )
        return best

    @staticmethod
    def _distance(a, b):
        return max(abs(a[i] - b[i]) for i in range(4))

    def update(self, frame):
        candidate = self._detect(frame)
        if candidate is None:
            self.missing += 1
            if self.missing >= PIPE_MISSING_RESET_FRAMES:
                self.samples = []
                self.axis = None
                self.locked = False
            return self.axis

        if not self.locked:
            self.missing = 0
            self.samples.append(candidate)
            if len(self.samples) > PIPE_LOCK_FRAMES:
                self.samples.pop(0)
            data = np.asarray(self.samples, dtype=np.float32)
            median = np.median(data, axis=0)
            self.axis = tuple(float(value) for value in median)
            self.locked = len(self.samples) >= PIPE_LOCK_FRAMES
            return self.axis

        if self._distance(candidate, self.axis) > PIPE_LOCK_MAX_JUMP_PX:
            self.missing += 1
            if self.missing >= PIPE_MISSING_RESET_FRAMES:
                self.samples = [candidate]
                self.axis = candidate
                self.locked = False
                self.missing = 0
            return self.axis

        self.missing = 0
        self.axis = tuple(
            self.axis[i] * (1.0 - PIPE_TRACK_ALPHA)
            + candidate[i] * PIPE_TRACK_ALPHA
            for i in range(6)
        )
        return self.axis


def object_center(obj):
    return float(obj.x + obj.w * 0.5), float(obj.y + obj.h * 0.5)


def project_to_pipe(x, y, axis):
    lift_x, lift_y, fixed_x, fixed_y, half_width, _ = axis
    dx = fixed_x - lift_x
    dy = fixed_y - lift_y
    length_sq = dx * dx + dy * dy
    if length_sq < 1.0:
        return None
    fraction = ((x - lift_x) * dx + (y - lift_y) * dy) / length_sq
    projected_x = lift_x + fraction * dx
    projected_y = lift_y + fraction * dy
    cross_track = math.hypot(x - projected_x, y - projected_y)
    return fraction, cross_track, max(1.0, half_width)


def select_ball(objects, axis, previous_position):
    best = None
    best_score = -1e9
    for obj in objects:
        if int(obj.class_id) != BALL_CLASS_ID:
            continue
        cx, cy = object_center(obj)
        projection = project_to_pipe(cx, cy, axis)
        if projection is None:
            continue
        fraction, cross_track, half_width = projection
        if (
            fraction < -BALL_END_MARGIN
            or fraction > 1.0 + BALL_END_MARGIN
            or cross_track > half_width + BALL_AXIS_MARGIN_PX
        ):
            continue
        position = clamp(fraction, 0.0, 1.0) * PIPE_LENGTH_CM
        score = float(obj.score) * 100.0 - cross_track * 0.20
        if previous_position is not None:
            score -= abs(position - previous_position) * 1.5
        if score > best_score:
            best_score = score
            best = (obj, position, cx, cy)
    return best


class BallEstimator:
    def __init__(self):
        self.history = []
        self.velocity = 0.0
        self.velocity_ready = False
        self.last_position = None
        self.last_ms = 0

    def clear(self):
        self.history = []
        self.velocity = 0.0
        self.velocity_ready = False
        self.last_position = None
        self.last_ms = 0

    def update(self, capture_ms, position_cm):
        if self.last_position is not None:
            dt_ms = elapsed_ms(capture_ms, self.last_ms)
            allowed = BALL_JUMP_ALLOWANCE_CM + BALL_MAX_SPEED_CM_S * dt_ms / 1000.0
            if dt_ms == 0 or dt_ms > 250 or abs(position_cm - self.last_position) > allowed:
                self.clear()

        self.last_position = float(position_cm)
        self.last_ms = int(capture_ms)
        self.history.append((int(capture_ms), float(position_cm)))
        self.history = [
            item for item in self.history
            if elapsed_ms(capture_ms, item[0]) <= VELOCITY_WINDOW_MS
        ]

        if len(self.history) < VELOCITY_MIN_SAMPLES:
            self.velocity_ready = False
            return 0.0, False
        span_ms = elapsed_ms(self.history[-1][0], self.history[0][0])
        if span_ms < VELOCITY_MIN_SPAN_MS:
            self.velocity_ready = False
            return 0.0, False

        t0 = self.history[0][0]
        times = np.asarray(
            [elapsed_ms(item[0], t0) / 1000.0 for item in self.history],
            dtype=np.float32,
        )
        positions = np.asarray([item[1] for item in self.history], dtype=np.float32)
        centered_time = times - float(times.mean())
        denominator = float(np.dot(centered_time, centered_time))
        if denominator < 1e-6:
            self.velocity_ready = False
            return 0.0, False
        raw_velocity = float(np.dot(centered_time, positions - positions.mean()) / denominator)
        raw_velocity = clamp(raw_velocity, -BALL_MAX_SPEED_CM_S, BALL_MAX_SPEED_CM_S)
        if self.velocity_ready:
            self.velocity += (raw_velocity - self.velocity) * VELOCITY_ALPHA
        else:
            self.velocity = raw_velocity
        self.velocity_ready = True
        return self.velocity, True


def draw_preview(img, axis, selected, position_cm, velocity_cm_s, pipe_locked):
    if axis is not None:
        lx, ly, fx, fy, _, _ = axis
        img.draw_line(
            int(lx), int(ly), int(fx), int(fy),
            color=image.COLOR_GREEN, thickness=3,
        )
        target_fraction = clamp(TARGET_POSITION_CM / PIPE_LENGTH_CM, 0.0, 1.0)
        tx = lx + (fx - lx) * target_fraction
        ty = ly + (fy - ly) * target_fraction
        img.draw_cross(
            int(tx), int(ty), color=image.COLOR_BLUE, size=12, thickness=3
        )
    if selected is not None:
        obj = selected[0]
        img.draw_rect(
            obj.x, obj.y, obj.w, obj.h,
            color=image.COLOR_RED, thickness=2,
        )
    label = "LOCK {}  X:{:.2f}  V:{:+.2f}".format(
        1 if pipe_locked else 0, position_cm, velocity_cm_s
    )
    img.draw_string(8, 8, label, color=image.COLOR_WHITE)


def main():
    serial = open_uart4()
    detector = nn.YOLO26(model=MODEL_PATH, dual_buff=False)
    cam = camera.Camera(
        detector.input_width(), detector.input_height(), detector.input_format()
    )
    screen = display.Display() if SHOW_PREVIEW else None
    pipe_tracker = PipeTracker()
    ball_estimator = BallEstimator()
    seq = 0
    last_send_ms = 0
    last_target_ms = 0

    serial.write(build_target_frame(TARGET_POSITION_CM))
    print("H balance vision started; 0 cm at lift end, target", TARGET_POSITION_CM)

    while not app.need_exit():
        img = cam.read()
        capture_ms = int(time.ticks_ms()) & 0xFFFFFFFF
        frame_cv = image.image2cv(img, copy=False)
        axis = pipe_tracker.update(frame_cv)
        objects = detector.detect(img, conf_th=BALL_CONFIDENCE, iou_th=BALL_IOU)
        selected = None
        position_cm = 0.0
        velocity_cm_s = 0.0
        velocity_valid = False
        confidence = 0.0

        if axis is not None and pipe_tracker.locked:
            selected = select_ball(objects, axis, ball_estimator.last_position)
        if selected is not None:
            obj, position_cm, _, _ = selected
            velocity_cm_s, velocity_valid = ball_estimator.update(
                capture_ms, position_cm
            )
            confidence = float(obj.score)
            valid = True
        else:
            valid = False
            ball_estimator.clear()

        now_ms = int(time.ticks_ms()) & 0xFFFFFFFF
        if elapsed_ms(now_ms, last_send_ms) >= UART_MIN_SEND_MS:
            processing_ms = elapsed_ms(now_ms, capture_ms)
            serial.write(
                build_ball_frame(
                    seq,
                    capture_ms,
                    position_cm,
                    velocity_cm_s,
                    confidence,
                    processing_ms,
                    valid,
                    velocity_valid,
                    pipe_tracker.locked,
                )
            )
            seq = (seq + 1) & 0xFFFF
            last_send_ms = now_ms

        if elapsed_ms(now_ms, last_target_ms) >= TARGET_REFRESH_MS:
            serial.write(build_target_frame(TARGET_POSITION_CM))
            last_target_ms = now_ms

        if screen is not None:
            draw_preview(
                img,
                axis,
                selected,
                position_cm,
                velocity_cm_s,
                pipe_tracker.locked,
            )
            screen.show(img)


if __name__ == "__main__":
    main()
