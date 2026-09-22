#include "h_balance_control.h"

#include <stddef.h>
#include <string.h>

#include "../motor/x42s_rs485/x42s_rs485.h"

static int32_t abs_i32(int32_t value)
{
    if (value == INT32_MIN) {
        return INT32_MAX;
    }
    return (value < 0) ? -value : value;
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static int64_t clamp_i64(int64_t value, int64_t minimum, int64_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static uint32_t isqrt_u32(uint32_t value)
{
    uint32_t result = 0U;
    uint32_t bit = 1UL << 30;

    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0U) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

static int32_t divide_round_nearest(int32_t numerator, int32_t denominator)
{
    if (denominator <= 0) {
        return 0;
    }
    if (numerator >= 0) {
        return (numerator + denominator / 2) / denominator;
    }
    return -((-numerator + denominator / 2) / denominator);
}

static int32_t limit_slew(int32_t desired, int32_t current)
{
    int32_t delta = desired - current;

    if (delta > H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE) {
        return current + H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE;
    }
    if (delta < -H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE) {
        return current - H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE;
    }
    return desired;
}

static int32_t compute_profile_acceleration(int32_t error_0p01cm,
                                            int32_t velocity_0p01cm_s,
                                            int32_t integral_acc_0p01cm_s2)
{
    uint64_t radicand;
    uint32_t root_input;
    int32_t distance_velocity;
    int32_t braking_velocity;
    int32_t reference_velocity;
    int32_t acceleration;

    radicand = 2ULL * (uint32_t)H_BALANCE_PROFILE_BRAKE_ACC_0P01CM_S2 *
                (uint32_t)abs_i32(error_0p01cm);
    root_input = (radicand > UINT32_MAX) ? UINT32_MAX : (uint32_t)radicand;
    braking_velocity = (int32_t)isqrt_u32(root_input);
    distance_velocity =
        (abs_i32(error_0p01cm) * H_BALANCE_PROFILE_POS_TO_VEL_NUM) /
        H_BALANCE_PROFILE_POS_TO_VEL_DEN;
    reference_velocity = clamp_i32(braking_velocity, 0,
                                   H_BALANCE_PROFILE_MAX_VEL_0P01CM_S);
    if (distance_velocity < reference_velocity) {
        reference_velocity = distance_velocity;
    }
    if (error_0p01cm < 0) {
        reference_velocity = -reference_velocity;
    }

    acceleration = H_BALANCE_PROFILE_VEL_KP *
                   (reference_velocity - velocity_0p01cm_s) +
                   integral_acc_0p01cm_s2;
    return clamp_i32(acceleration, -H_BALANCE_MAX_ACC_0P01CM_S2,
                     H_BALANCE_MAX_ACC_0P01CM_S2);
}

int32_t HBalance_ComputeTilt0p1Deg(int32_t target_0p01cm,
                                   int32_t position_0p01cm,
                                   int32_t velocity_0p01cm_s,
                                   int32_t integral_acc_0p01cm_s2)
{
    int32_t error = target_0p01cm - position_0p01cm;
    int32_t acceleration;
    int32_t tilt;

    if (position_0p01cm <= H_BALANCE_EDGE_GUARD_0P01CM) {
        return H_BALANCE_EDGE_TILT_0P1DEG;
    }
    if (position_0p01cm >=
        H_BALANCE_PIPE_MAX_0P01CM - H_BALANCE_EDGE_GUARD_0P01CM) {
        return -H_BALANCE_EDGE_TILT_0P1DEG;
    }

    acceleration = compute_profile_acceleration(
        error, velocity_0p01cm_s, integral_acc_0p01cm_s2);

    tilt = divide_round_nearest(acceleration,
                                H_BALANCE_ACC_PER_0P1DEG_0P01CM_S2);
    return clamp_i32(tilt, -H_BALANCE_MAX_TILT_0P1DEG,
                     H_BALANCE_MAX_TILT_0P1DEG);
}

static bool ball_sample_is_usable(const MaixCAM_BallState *ball)
{
    if (ball == NULL || !ball->valid || !ball->pipe_locked) {
        return false;
    }
    if (ball->predicted ||
        ball->confidence_0p01pct < H_BALANCE_CONFIDENCE_MIN_0P01PCT) {
        return false;
    }
    return ball->position_0p01cm >= H_BALANCE_PIPE_MIN_0P01CM &&
           ball->position_0p01cm <= H_BALANCE_PIPE_MAX_0P01CM;
}

static void reset_estimator(HBalance_Control *control)
{
    control->estimated_position_0p01cm = 0;
    control->estimated_velocity_0p01cm_s = 0;
    control->integral_error_0p01cm_ms = 0;
    control->last_capture_ms = 0U;
    control->valid_frame_count = 0U;
    control->estimator_ready = false;
    control->has_ball_sample = false;
}

static void stop_at_level(HBalance_Control *control, uint32_t now_ms)
{
    int32_t level_position;

    if (!control->armed ||
        (uint32_t)(now_ms - control->last_bus_tx_ms) <
        H_BALANCE_BUS_GUARD_MS) {
        return;
    }

    level_position = control->level_zero_0p1deg;
    if (control->commanded_position_0p1deg != level_position ||
        (uint32_t)(now_ms - control->last_motor_command_ms) >=
        H_BALANCE_COMMAND_REFRESH_MS) {
        X42S_SetPositionEx(H_BALANCE_MOTOR_ID, level_position,
                          H_BALANCE_MOTOR_ACC_RPM_S,
                          H_BALANCE_MOTOR_ACC_RPM_S,
                          H_BALANCE_MOTOR_SPEED_MIN_0P1RPM,
                          0U, false);
        control->commanded_position_0p1deg = level_position;
        control->requested_tilt_0p1deg = 0;
        control->last_motor_command_ms = now_ms;
        control->last_bus_tx_ms = now_ms;
    }
}

static void enter_fault(HBalance_Control *control, HBalance_Fault fault)
{
    if (control->state != H_BALANCE_STATE_FAULT) {
        X42S_Stop(H_BALANCE_MOTOR_ID);
    }
    control->state = H_BALANCE_STATE_FAULT;
    control->fault = fault;
    control->requested_tilt_0p1deg = 0;
    control->integral_error_0p01cm_ms = 0;
}

static void update_motor_feedback(HBalance_Control *control, uint32_t now_ms)
{
    uint16_t count = X42S_GetPositionUpdateCount(H_BALANCE_MOTOR_ID);

    if (count == control->motor_update_count) {
        return;
    }
    control->motor_update_count = count;
    control->motor_position_0p1deg =
        X42S_GetLastPosition(H_BALANCE_MOTOR_ID);
    control->last_motor_feedback_ms = now_ms;
}

static bool update_estimator(HBalance_Control *control,
                             const MaixCAM_BallState *ball,
                             uint32_t now_ms)
{
    uint32_t dt_ms;
    int32_t measured_position;
    int32_t predicted_position;
    int32_t residual;
    int32_t residual_velocity;
    int32_t fused_velocity;

    if (!ball_sample_is_usable(ball)) {
        return false;
    }
    if ((uint32_t)(now_ms - ball->timestamp_ms) >
        H_BALANCE_VISION_GRACE_MS) {
        return false;
    }
    if (control->has_ball_sample && ball->seq == control->last_ball_seq &&
        ball->capture_ms == control->last_capture_ms) {
        return false;
    }

    measured_position = ball->position_0p01cm;
    if (ball->velocity_valid && ball->processing_ms <= 200U) {
        measured_position +=
            ((int32_t)ball->velocity_0p01cm_s * (int32_t)ball->processing_ms) /
            1000;
        measured_position = clamp_i32(measured_position,
                                      H_BALANCE_PIPE_MIN_0P01CM,
                                      H_BALANCE_PIPE_MAX_0P01CM);
    }

    if (!control->estimator_ready) {
        control->estimated_position_0p01cm = measured_position;
        control->estimated_velocity_0p01cm_s =
            ball->velocity_valid ? ball->velocity_0p01cm_s : 0;
        control->estimator_ready = true;
        control->valid_frame_count = 1U;
    } else {
        dt_ms = ball->capture_ms - control->last_capture_ms;
        if (dt_ms < H_BALANCE_CAPTURE_DT_MIN_MS ||
            dt_ms > H_BALANCE_CAPTURE_DT_MAX_MS) {
            control->estimated_position_0p01cm = measured_position;
            control->estimated_velocity_0p01cm_s =
                ball->velocity_valid ? ball->velocity_0p01cm_s : 0;
            control->valid_frame_count = 1U;
            control->integral_error_0p01cm_ms = 0;
        } else {
            predicted_position = control->estimated_position_0p01cm +
                (control->estimated_velocity_0p01cm_s * (int32_t)dt_ms) /
                1000;
            residual = measured_position - predicted_position;
            control->estimated_position_0p01cm = predicted_position +
                (residual * H_BALANCE_ESTIMATOR_ALPHA_NUM) /
                H_BALANCE_ESTIMATOR_ALPHA_DEN;
            residual_velocity = (residual * 1000) / (int32_t)dt_ms;
            control->estimated_velocity_0p01cm_s +=
                (residual_velocity * H_BALANCE_ESTIMATOR_BETA_NUM) /
                H_BALANCE_ESTIMATOR_BETA_DEN;
            if (ball->velocity_valid) {
                fused_velocity =
                    control->estimated_velocity_0p01cm_s *
                    (H_BALANCE_VISION_VELOCITY_FUSE_DEN -
                     H_BALANCE_VISION_VELOCITY_FUSE_NUM) +
                    (int32_t)ball->velocity_0p01cm_s *
                    H_BALANCE_VISION_VELOCITY_FUSE_NUM;
                control->estimated_velocity_0p01cm_s =
                    fused_velocity / H_BALANCE_VISION_VELOCITY_FUSE_DEN;
            }
            control->estimated_velocity_0p01cm_s = clamp_i32(
                control->estimated_velocity_0p01cm_s,
                -H_BALANCE_MAX_BALL_SPEED_0P01CM_S,
                H_BALANCE_MAX_BALL_SPEED_0P01CM_S);
            if (control->valid_frame_count < UINT8_MAX) {
                control->valid_frame_count++;
            }
        }
    }

    control->last_ball_seq = ball->seq;
    control->last_capture_ms = ball->capture_ms;
    control->last_valid_ball_ms = now_ms;
    control->has_ball_sample = true;
    return true;
}

static int32_t update_integral(HBalance_Control *control, uint32_t dt_ms)
{
    int32_t error = control->target_0p01cm -
                    control->estimated_position_0p01cm;
    int64_t integral_acc;

    if (abs_i32(error) > H_BALANCE_INTEGRAL_BAND_0P01CM) {
        control->integral_error_0p01cm_ms =
            (control->integral_error_0p01cm_ms * 7) / 8;
        return 0;
    }

    control->integral_error_0p01cm_ms += (int64_t)error * dt_ms;
    control->integral_error_0p01cm_ms = clamp_i64(
        control->integral_error_0p01cm_ms,
        -H_BALANCE_INTEGRAL_MAX_0P01CM_MS,
        H_BALANCE_INTEGRAL_MAX_0P01CM_MS);
    integral_acc = control->integral_error_0p01cm_ms *
                   H_BALANCE_INTEGRAL_KI_NUM /
                   H_BALANCE_INTEGRAL_KI_DEN;
    return clamp_i32((int32_t)integral_acc,
                     -H_BALANCE_INTEGRAL_ACC_MAX_0P01CM_S2,
                     H_BALANCE_INTEGRAL_ACC_MAX_0P01CM_S2);
}

static void update_settle_state(HBalance_Control *control, uint32_t now_ms)
{
    int32_t error = control->target_0p01cm -
                    control->estimated_position_0p01cm;

    if (abs_i32(error) <= H_BALANCE_SETTLE_POS_0P01CM &&
        abs_i32(control->estimated_velocity_0p01cm_s) <=
        H_BALANCE_SETTLE_VEL_0P01CM_S) {
        if (control->settle_started_ms == 0U) {
            control->settle_started_ms = now_ms;
        } else if ((uint32_t)(now_ms - control->settle_started_ms) >=
                   H_BALANCE_SETTLE_HOLD_MS) {
            control->state = H_BALANCE_STATE_SETTLED;
        }
    } else {
        control->settle_started_ms = 0U;
        control->state = H_BALANCE_STATE_ACTIVE;
    }
}

static bool service_motor_read(HBalance_Control *control, uint32_t now_ms)
{
    if ((uint32_t)(now_ms - control->last_motor_request_ms) <
        H_BALANCE_MOTOR_READ_PERIOD_MS ||
        (uint32_t)(now_ms - control->last_bus_tx_ms) <
        H_BALANCE_BUS_GUARD_MS) {
        return false;
    }

    (void)X42S_ReadPosition(H_BALANCE_MOTOR_ID);
    control->last_motor_request_ms = now_ms;
    control->last_bus_tx_ms = now_ms;
    return true;
}

static void send_tilt_command(HBalance_Control *control, int32_t desired_tilt,
                              uint32_t now_ms)
{
    int32_t absolute_target;
    int32_t delta;
    uint32_t speed;

    desired_tilt = clamp_i32(desired_tilt, -H_BALANCE_MAX_TILT_0P1DEG,
                             H_BALANCE_MAX_TILT_0P1DEG);
    desired_tilt = limit_slew(desired_tilt,
                              control->requested_tilt_0p1deg);
    absolute_target = control->level_zero_0p1deg + desired_tilt;
    delta = absolute_target - control->commanded_position_0p1deg;

    if (abs_i32(delta) < H_BALANCE_COMMAND_HYST_0P1DEG &&
        (uint32_t)(now_ms - control->last_motor_command_ms) <
        H_BALANCE_COMMAND_REFRESH_MS) {
        control->requested_tilt_0p1deg = desired_tilt;
        return;
    }
    if ((uint32_t)(now_ms - control->last_bus_tx_ms) <
        H_BALANCE_BUS_GUARD_MS) {
        return;
    }

    speed = (uint32_t)abs_i32(absolute_target -
                               control->motor_position_0p1deg) *
            H_BALANCE_MOTOR_SPEED_PER_ERROR;
    if (speed < H_BALANCE_MOTOR_SPEED_MIN_0P1RPM) {
        speed = H_BALANCE_MOTOR_SPEED_MIN_0P1RPM;
    }
    if (speed > H_BALANCE_MOTOR_SPEED_MAX_0P1RPM) {
        speed = H_BALANCE_MOTOR_SPEED_MAX_0P1RPM;
    }

    X42S_SetPositionEx(H_BALANCE_MOTOR_ID, absolute_target,
                       H_BALANCE_MOTOR_ACC_RPM_S,
                       H_BALANCE_MOTOR_ACC_RPM_S,
                       (uint16_t)speed, 0U, false);
    control->requested_tilt_0p1deg = desired_tilt;
    control->commanded_position_0p1deg = absolute_target;
    control->last_motor_command_ms = now_ms;
    control->last_bus_tx_ms = now_ms;
}

void HBalance_Init(HBalance_Control *control)
{
    if (control == NULL) {
        return;
    }
    memset(control, 0, sizeof(*control));
    control->state = H_BALANCE_STATE_DISABLED;
    control->target_0p01cm = H_BALANCE_DEFAULT_TARGET_0P01CM;
    control->motor_update_count =
        X42S_GetPositionUpdateCount(H_BALANCE_MOTOR_ID);
}

bool HBalance_SetTarget(HBalance_Control *control, int32_t target_0p01cm)
{
    if (control == NULL || target_0p01cm < H_BALANCE_TARGET_MIN_0P01CM ||
        target_0p01cm > H_BALANCE_TARGET_MAX_0P01CM) {
        if (control != NULL) {
            control->fault = H_BALANCE_FAULT_INVALID_TARGET;
        }
        return false;
    }
    if (control->target_0p01cm == target_0p01cm) {
        return true;
    }
    control->target_0p01cm = target_0p01cm;
    control->integral_error_0p01cm_ms = 0;
    control->settle_started_ms = 0U;
    return true;
}

void HBalance_RequestArm(HBalance_Control *control, uint32_t now_ms)
{
    if (control == NULL) {
        return;
    }
    if (control->armed || control->state == H_BALANCE_STATE_WAIT_MOTOR ||
        control->state == H_BALANCE_STATE_WAIT_VISION) {
        HBalance_Disarm(control);
        return;
    }
#if H_BALANCE_MOTION_ENABLED == 0U
    (void)now_ms;
    return;
#else
    control->fault = H_BALANCE_FAULT_NONE;
    control->state = H_BALANCE_STATE_WAIT_MOTOR;
    control->arm_started_ms = now_ms;
    control->last_update_ms = now_ms;
    control->last_motor_feedback_ms = 0U;
    control->motor_update_count =
        X42S_GetPositionUpdateCount(H_BALANCE_MOTOR_ID);
    control->last_motor_request_ms = now_ms;
    control->last_bus_tx_ms = now_ms;
    reset_estimator(control);
    (void)X42S_ReadPosition(H_BALANCE_MOTOR_ID);
#endif
}

void HBalance_Disarm(HBalance_Control *control)
{
    if (control == NULL) {
        return;
    }
    if (control->armed || control->state == H_BALANCE_STATE_WAIT_MOTOR ||
        control->state == H_BALANCE_STATE_WAIT_VISION) {
        X42S_Stop(H_BALANCE_MOTOR_ID);
    }
    control->armed = false;
    control->state = H_BALANCE_STATE_DISABLED;
    control->fault = H_BALANCE_FAULT_NONE;
    control->requested_tilt_0p1deg = 0;
    control->integral_error_0p01cm_ms = 0;
    control->settle_started_ms = 0U;
    reset_estimator(control);
}

void HBalance_SetArmKey(HBalance_Control *control, bool pressed,
                        uint32_t now_ms)
{
    if (control == NULL) {
        return;
    }

    if (!control->key_armed) {
        if (pressed) {
            control->key_release_seen = false;
        } else if (!control->key_release_seen) {
            control->key_release_seen = true;
            control->key_released_since_ms = now_ms;
        } else if ((uint32_t)(now_ms - control->key_released_since_ms) >=
                   H_BALANCE_ARM_RELEASE_MS) {
            control->key_armed = true;
            control->key_raw_pressed = false;
            control->key_stable_pressed = false;
            control->key_raw_since_ms = now_ms;
        }
        return;
    }

    if (pressed != control->key_raw_pressed) {
        control->key_raw_pressed = pressed;
        control->key_raw_since_ms = now_ms;
    }
    if (pressed != control->key_stable_pressed &&
        (uint32_t)(now_ms - control->key_raw_since_ms) >=
        H_BALANCE_ARM_DEBOUNCE_MS) {
        control->key_stable_pressed = pressed;
        if (pressed) {
            HBalance_RequestArm(control, now_ms);
        }
    }
}

void HBalance_Update(HBalance_Control *control,
                     const MaixCAM_Parser *vision, uint32_t now_ms)
{
    uint32_t dt_ms;
    int32_t relative_motor_position;
    int32_t integral_acc;
    int32_t desired_tilt;

    if (control == NULL || vision == NULL) {
        return;
    }

    update_motor_feedback(control, now_ms);
    if (vision->target_position.timestamp_ms != 0U &&
        vision->target_position.timestamp_ms != control->target_timestamp_ms) {
        control->target_timestamp_ms = vision->target_position.timestamp_ms;
        if (!HBalance_SetTarget(
                control, vision->target_position.position_0p01cm)) {
            if (control->state != H_BALANCE_STATE_DISABLED) {
                enter_fault(control, H_BALANCE_FAULT_INVALID_TARGET);
            }
            return;
        }
    }

    if (control->state == H_BALANCE_STATE_DISABLED ||
        control->state == H_BALANCE_STATE_FAULT) {
        return;
    }

    if ((uint32_t)(now_ms - control->arm_started_ms) >
        H_BALANCE_ARM_TIMEOUT_MS && !control->armed) {
        enter_fault(control, H_BALANCE_FAULT_ARM_TIMEOUT);
        return;
    }

    if (control->state == H_BALANCE_STATE_WAIT_MOTOR) {
        if (control->last_motor_feedback_ms == 0U) {
            (void)service_motor_read(control, now_ms);
            return;
        }
        control->level_zero_0p1deg = control->motor_position_0p1deg;
        control->commanded_position_0p1deg = control->level_zero_0p1deg;
        control->requested_tilt_0p1deg = 0;
        X42S_Enable(H_BALANCE_MOTOR_ID);
        control->last_bus_tx_ms = now_ms;
        control->armed = true;
        control->state = H_BALANCE_STATE_WAIT_VISION;
        return;
    }

    relative_motor_position = control->motor_position_0p1deg -
                              control->level_zero_0p1deg;
    if (abs_i32(relative_motor_position) >
        H_BALANCE_MAX_TILT_0P1DEG + H_BALANCE_MOTOR_RANGE_MARGIN_0P1DEG) {
        enter_fault(control, H_BALANCE_FAULT_MOTOR_RANGE);
        return;
    }
    if (control->last_motor_feedback_ms != 0U &&
        (uint32_t)(now_ms - control->last_motor_feedback_ms) >
        H_BALANCE_MOTOR_FEEDBACK_TIMEOUT_MS) {
        enter_fault(control, H_BALANCE_FAULT_MOTOR_FEEDBACK_TIMEOUT);
        return;
    }

    (void)update_estimator(control, &vision->ball, now_ms);
    if (!control->estimator_ready ||
        (uint32_t)(now_ms - control->last_valid_ball_ms) >
        H_BALANCE_VISION_GRACE_MS) {
        control->state = H_BALANCE_STATE_VISION_LOST;
        control->valid_frame_count = 0U;
        control->integral_error_0p01cm_ms = 0;
        if ((uint32_t)(now_ms - control->last_valid_ball_ms) >
            H_BALANCE_VISION_TIMEOUT_MS) {
            reset_estimator(control);
        }
        if (!service_motor_read(control, now_ms)) {
            stop_at_level(control, now_ms);
        }
        return;
    }
    if (control->valid_frame_count < H_BALANCE_VALID_FRAMES_TO_RUN) {
        control->state = H_BALANCE_STATE_WAIT_VISION;
        if (!service_motor_read(control, now_ms)) {
            stop_at_level(control, now_ms);
        }
        return;
    }

    if ((uint32_t)(now_ms - control->last_update_ms) <
        H_BALANCE_CONTROL_PERIOD_MS) {
        return;
    }
    dt_ms = now_ms - control->last_update_ms;
    control->last_update_ms = now_ms;

    if (service_motor_read(control, now_ms)) {
        return;
    }

    integral_acc = update_integral(control, dt_ms);
    desired_tilt = HBalance_ComputeTilt0p1Deg(
        control->target_0p01cm,
        control->estimated_position_0p01cm,
        control->estimated_velocity_0p01cm_s,
        integral_acc);
    send_tilt_command(control, desired_tilt, now_ms);
    update_settle_state(control, now_ms);
}
