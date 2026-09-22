#include "car_line_follow.h"

#include "car_gray.h"
#include "../config/app_config.h"

typedef enum {
    CAR_LINE_STATE_FOLLOW = 0U,
    CAR_LINE_STATE_WAIT_ZERO,
    CAR_LINE_STATE_TURN_DELAY,
    CAR_LINE_STATE_RIGHT_TURN
} CarLineState;

#define CAR_LINE_ALL_SENSOR_ON_BITS ((uint8_t)((1U << CAR_GRAY_SENSOR_NUM) - 1U))
#define CAR_LINE_RIGHT_TURN_DELAY_TICKS \
    ((uint16_t)(((uint32_t)CAR_LINE_RIGHT_TURN_DELAY_MS + \
    CAR_ENCODER_SAMPLE_MS - 1U) / CAR_ENCODER_SAMPLE_MS))

static volatile int16_t g_line_correction;
static volatile int32_t g_line_correction_mmps;
static volatile int32_t g_line_delta_a;
static volatile int32_t g_line_delta_b;
static volatile int32_t g_line_slowdown_mmps;
static volatile int16_t g_last_valid_correction;
static volatile CarLineState g_line_state = CAR_LINE_STATE_FOLLOW;
static volatile uint16_t g_line_turn_delay_ticks;

static int32_t limit_i32(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value > max_value) {
        return max_value;
    }
    if (value < min_value) {
        return min_value;
    }
    return value;
}

static int32_t apply_forward_sign(int32_t forward_speed, int32_t sign)
{
    return (sign < 0) ? -forward_speed : forward_speed;
}

static int32_t abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t apply_curve_speed_limit(int32_t base_target, int32_t delta)
{
    int32_t direction = (base_target < 0) ? -1 : 1;
    int32_t magnitude = abs_i32(base_target);
    int32_t target;

    magnitude -= g_line_slowdown_mmps;
    if (magnitude < CAR_LINE_MIN_WHEEL_SPEED_MMPS) {
        magnitude = CAR_LINE_MIN_WHEEL_SPEED_MMPS;
    }
    target = (direction * magnitude) + delta;

    // During a fast curve, preserve a rolling inner wheel instead of letting
    // a large correction command it into reverse and destabilize the chassis.
    if ((direction > 0) && (target < CAR_LINE_MIN_WHEEL_SPEED_MMPS)) {
        target = CAR_LINE_MIN_WHEEL_SPEED_MMPS;
    } else if ((direction < 0) && (target > -CAR_LINE_MIN_WHEEL_SPEED_MMPS)) {
        target = -CAR_LINE_MIN_WHEEL_SPEED_MMPS;
    }

    return target;
}

void CarLineFollow_Init(void)
{
    g_line_correction = 0;
    g_line_correction_mmps = 0;
    g_line_delta_a = 0;
    g_line_delta_b = 0;
    g_line_slowdown_mmps = 0;
    g_last_valid_correction = 0;
    g_line_state = CAR_LINE_STATE_FOLLOW;
    g_line_turn_delay_ticks = 0;
}

void CarLineFollow_Update(void)
{
    uint8_t bits;
#if CAR_LINE_SPECIAL_RIGHT_TURN_ENABLED
    uint8_t out3;
    uint8_t out4;
    uint8_t out5;
    uint8_t out6;
    uint8_t out7;

    if (g_line_state == CAR_LINE_STATE_TURN_DELAY) {
        g_line_correction = 0;
        g_line_correction_mmps = 0;
        g_line_delta_a = 0;
        g_line_delta_b = 0;

        if (g_line_turn_delay_ticks > 0U) {
            g_line_turn_delay_ticks--;
        }
        if (g_line_turn_delay_ticks == 0U) {
            g_line_state = CAR_LINE_STATE_RIGHT_TURN;
        }
        return;
    }
#endif

    g_line_correction = CarGray_CalculateCorrection();
    bits = CarGray_GetBits();
#if CAR_LINE_SPECIAL_RIGHT_TURN_ENABLED
    out3 = CarGray_GetValue(2U);
    out4 = CarGray_GetValue(3U);
    out5 = CarGray_GetValue(4U);
    out6 = CarGray_GetValue(5U);
    out7 = CarGray_GetValue(6U);

    switch (g_line_state) {
    case CAR_LINE_STATE_FOLLOW:
        if ((out5 != 0U) && (out6 != 0U) && (out7 != 0U)) {
            g_line_state = CAR_LINE_STATE_WAIT_ZERO;
            g_line_correction = 0;
            g_line_correction_mmps = 0;
            g_line_delta_a = 0;
            g_line_delta_b = 0;
            return;
        }
        break;

    case CAR_LINE_STATE_WAIT_ZERO:
        g_line_correction = 0;
        g_line_correction_mmps = 0;
        g_line_delta_a = 0;
        g_line_delta_b = 0;
        if ((bits == 0U) || (bits == CAR_LINE_ALL_SENSOR_ON_BITS)) {
            g_line_state = CAR_LINE_STATE_TURN_DELAY;
            g_line_turn_delay_ticks = CAR_LINE_RIGHT_TURN_DELAY_TICKS;
        }
        return;

    case CAR_LINE_STATE_RIGHT_TURN:
        g_line_correction = 0;
        g_line_correction_mmps = 0;
        g_line_delta_a = 0;
        g_line_delta_b = 0;
        if ((out3 != 0U) || (out4 != 0U) || (out5 != 0U)) {
            g_line_state = CAR_LINE_STATE_FOLLOW;
        }
        return;

    default:
        g_line_state = CAR_LINE_STATE_FOLLOW;
        g_line_turn_delay_ticks = 0;
        return;
    }
#endif

    if (bits == 0U) {
        // Keep steering toward the last observed line side for one control
        // cycle instead of commanding straight ahead the instant it is lost.
        g_line_correction = g_last_valid_correction;
    } else {
        g_last_valid_correction = g_line_correction;
    }

    g_line_correction_mmps =
        ((int32_t)g_line_correction * (int32_t)CAR_LINE_TURN_SIGN) /
        (int32_t)CAR_LINE_CORRECTION_DIVISOR;
    g_line_correction_mmps = limit_i32(g_line_correction_mmps,
        -(int32_t)CAR_LINE_CORRECTION_LIMIT_MMPS,
        (int32_t)CAR_LINE_CORRECTION_LIMIT_MMPS);
    g_line_slowdown_mmps = abs_i32(g_line_correction_mmps) /
        CAR_LINE_CURVE_SLOWDOWN_DIVISOR;
    g_line_slowdown_mmps = limit_i32(g_line_slowdown_mmps, 0,
        CAR_LINE_CURVE_SLOWDOWN_MAX_MMPS);

    g_line_delta_a = apply_forward_sign(g_line_correction_mmps,
                                        (int32_t)CAR_LINE_A_FORWARD_SIGN);
    g_line_delta_b = -apply_forward_sign(g_line_correction_mmps,
                                         (int32_t)CAR_LINE_B_FORWARD_SIGN);
}

int16_t CarLineFollow_GetCorrection(void)
{
    return g_line_correction;
}

int32_t CarLineFollow_GetCorrectionMmps(void)
{
    return g_line_correction_mmps;
}

int32_t CarLineFollow_ApplyCorrectionA(int32_t base_target_a)
{
    if (g_line_state == CAR_LINE_STATE_RIGHT_TURN) {
        return CAR_LINE_RIGHT_TURN_LEFT_MMPS;
    }
    return apply_curve_speed_limit(base_target_a, g_line_delta_a);
}

int32_t CarLineFollow_ApplyCorrectionB(int32_t base_target_b)
{
    if (g_line_state == CAR_LINE_STATE_RIGHT_TURN) {
        return 0;
    }
    return apply_curve_speed_limit(base_target_b, g_line_delta_b);
}

uint8_t CarLineFollow_IsRightTurning(void)
{
    return (g_line_state == CAR_LINE_STATE_RIGHT_TURN) ? 1U : 0U;
}
