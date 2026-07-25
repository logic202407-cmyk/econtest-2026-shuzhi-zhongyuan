#include "gimbal_control.h"

#include "../motor/x42s_rs485/x42s_rs485.h"

#if (GIMBAL_MOTION_ENABLED != 0U) || (GIMBAL_YAW_ONLY_TEST_ENABLED != 0U)
static int32_t clamp_i32(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }

    if (value > max_value) {
        return max_value;
    }

    return value;
}

static int32_t limit_step(int32_t step)
{
    return clamp_i32(step, -GIMBAL_STEP_LIMIT_0P1DEG,
                     GIMBAL_STEP_LIMIT_0P1DEG);
}

static int32_t calc_step_0p1deg(int16_t error_0p01deg,
                                int32_t kp_num, int32_t kp_den)
{
    int32_t motor_error_0p1deg;
    int32_t step_0p1deg;

    if (kp_den == 0) {
        return 0;
    }

    motor_error_0p1deg = ((int32_t)error_0p01deg * 10) /
                         (int32_t)VISION_ANGLE_UNITS_PER_DEG;
    step_0p1deg = (motor_error_0p1deg * kp_num) / kp_den;
    return limit_step(step_0p1deg);
}

static int32_t limit_yaw_only_step(int32_t step)
{
    return clamp_i32(step, -GIMBAL_YAW_ONLY_STEP_LIMIT_0P1DEG,
                     GIMBAL_YAW_ONLY_STEP_LIMIT_0P1DEG);
}

static int32_t calc_yaw_only_correction_0p1deg(int32_t error_0p01deg)
{
    int32_t motor_error_0p1deg;
    int32_t correction_0p1deg;

    if (error_0p01deg > -GIMBAL_YAW_ONLY_DEADBAND_0P01DEG &&
        error_0p01deg < GIMBAL_YAW_ONLY_DEADBAND_0P01DEG) {
        return 0;
    }

    if (GIMBAL_YAW_ONLY_ANGLE_GAIN_DEN == 0) {
        return 0;
    }

    motor_error_0p1deg = (error_0p01deg * 10) /
                         (int32_t)VISION_ANGLE_UNITS_PER_DEG;
    correction_0p1deg =
        (motor_error_0p1deg * GIMBAL_YAW_ONLY_ANGLE_GAIN_NUM) /
        GIMBAL_YAW_ONLY_ANGLE_GAIN_DEN;
    return limit_yaw_only_step(correction_0p1deg);
}

static int32_t limit_yaw_only_speed(int32_t speed_0p1rpm)
{
    return clamp_i32(speed_0p1rpm,
                     -GIMBAL_YAW_ONLY_SPEED_MAX_0P1_RPM,
                     GIMBAL_YAW_ONLY_SPEED_MAX_0P1_RPM);
}

static int32_t calc_yaw_only_speed_0p1rpm(Gimbal_Control *gimbal,
                                          int32_t error_0p01deg)
{
    int32_t previous_error = gimbal->previous_yaw_error_0p01deg;
    int32_t derivative_0p01deg;
    int32_t speed_0p1rpm;

    if ((previous_error > 0 && error_0p01deg < 0) ||
        (previous_error < 0 && error_0p01deg > 0)) {
        gimbal->previous_yaw_error_0p01deg = error_0p01deg;
        return 0;
    }

    if (error_0p01deg > -GIMBAL_YAW_ONLY_DEADBAND_0P01DEG &&
        error_0p01deg < GIMBAL_YAW_ONLY_DEADBAND_0P01DEG) {
        gimbal->previous_yaw_error_0p01deg = error_0p01deg;
        return 0;
    }

    if (GIMBAL_YAW_ONLY_SPEED_KP_DEN == 0 ||
        GIMBAL_YAW_ONLY_SPEED_KD_DEN == 0) {
        return 0;
    }

    derivative_0p01deg = error_0p01deg - previous_error;
    gimbal->previous_yaw_error_0p01deg = error_0p01deg;

    speed_0p1rpm = (error_0p01deg * GIMBAL_YAW_ONLY_SPEED_KP_NUM) /
                   GIMBAL_YAW_ONLY_SPEED_KP_DEN;
    speed_0p1rpm += (derivative_0p01deg * GIMBAL_YAW_ONLY_SPEED_KD_NUM) /
                    GIMBAL_YAW_ONLY_SPEED_KD_DEN;

    if (speed_0p1rpm > 0 &&
        speed_0p1rpm < GIMBAL_YAW_ONLY_SPEED_MIN_0P1_RPM) {
        speed_0p1rpm = GIMBAL_YAW_ONLY_SPEED_MIN_0P1_RPM;
    } else if (speed_0p1rpm < 0 &&
               speed_0p1rpm > -GIMBAL_YAW_ONLY_SPEED_MIN_0P1_RPM) {
        speed_0p1rpm = -GIMBAL_YAW_ONLY_SPEED_MIN_0P1_RPM;
    }

    return limit_yaw_only_speed(speed_0p1rpm);
}

static int32_t limit_yaw_only_speed_step(int32_t desired_speed,
                                         int32_t current_speed)
{
    int32_t delta = desired_speed - current_speed;

    if (desired_speed == 0) {
        return 0;
    }

    if (delta > GIMBAL_YAW_ONLY_SPEED_STEP_0P1_RPM) {
        return current_speed + GIMBAL_YAW_ONLY_SPEED_STEP_0P1_RPM;
    }

    if (delta < -GIMBAL_YAW_ONLY_SPEED_STEP_0P1_RPM) {
        return current_speed - GIMBAL_YAW_ONLY_SPEED_STEP_0P1_RPM;
    }

    return desired_speed;
}

static bool update_yaw_only_position_feedback(Gimbal_Control *gimbal,
                                              uint32_t now_ms)
{
    uint16_t count = X42S_GetPositionUpdateCount(X42S_YAW_MOTOR_ID);

    if (count != gimbal->last_position_update_count) {
        gimbal->last_position_update_count = count;
        gimbal->last_position_feedback_ms = now_ms;
        gimbal->yaw_position_0p1deg = X42S_GetLastPosition(X42S_YAW_MOTOR_ID);
    }

    if ((uint32_t)(now_ms - gimbal->last_position_request_ms) >=
        GIMBAL_YAW_ONLY_POS_REQ_MS) {
        X42S_ReadPosition(X42S_YAW_MOTOR_ID);
        gimbal->last_position_request_ms = now_ms;
    }

    return gimbal->last_position_feedback_ms != 0U &&
           (uint32_t)(now_ms - gimbal->last_position_feedback_ms) <=
           GIMBAL_YAW_ONLY_POS_FEEDBACK_TIMEOUT_MS;
}

static int32_t filter_yaw_error_0p01deg(Gimbal_Control *gimbal,
                                        int16_t yaw_0p01deg)
{
    int32_t yaw = (int32_t)yaw_0p01deg;

    if (!gimbal->filter_ready) {
        gimbal->filtered_yaw_0p01deg = yaw;
        gimbal->filter_ready = true;
        return yaw;
    }

    gimbal->filtered_yaw_0p01deg +=
        (yaw - gimbal->filtered_yaw_0p01deg) >>
        GIMBAL_YAW_ONLY_FILTER_SHIFT;
    return gimbal->filtered_yaw_0p01deg;
}

static void stop_yaw_tracking(Gimbal_Control *gimbal, uint32_t now_ms,
                              bool force)
{
    bool should_send_stop = !gimbal->stopped ||
                            gimbal->yaw_speed_0p1rpm != 0 ||
                            (force && gimbal->stop_refresh_count == 0U);

    if (!force && !should_send_stop && gimbal->stop_refresh_count < 3U &&
        (uint32_t)(now_ms - gimbal->last_stop_ms) >=
        GIMBAL_YAW_ONLY_STOP_REFRESH_MS) {
        should_send_stop = true;
    }

    if (should_send_stop) {
        X42S_SetSpeedEx(X42S_YAW_MOTOR_ID, 0, GIMBAL_YAW_ONLY_ACC_RPM_S,
                        false);
        if (force) {
            X42S_Stop(X42S_YAW_MOTOR_ID);
        }
        gimbal->last_stop_ms = now_ms;
        if (gimbal->stop_refresh_count < 3U) {
            gimbal->stop_refresh_count++;
        }
    }

    gimbal->stopped = true;
    gimbal->filter_ready = false;
    gimbal->previous_yaw_error_0p01deg = 0;
    gimbal->yaw_speed_0p1rpm = 0;
}
#endif

void Gimbal_Init(Gimbal_Control *gimbal)
{
    if (gimbal == NULL) {
        return;
    }

    gimbal->yaw_position_0p1deg = 0;
    gimbal->pitch_position_0p1deg = 0;
    gimbal->filtered_yaw_0p01deg = 0;
    gimbal->previous_yaw_error_0p01deg = 0;
    gimbal->yaw_speed_0p1rpm = 0;
    gimbal->last_update_ms = 0U;
    gimbal->last_motion_ms = 0U;
    gimbal->last_position_request_ms = 0U;
    gimbal->last_position_feedback_ms = 0U;
    gimbal->last_stop_ms = 0U;
    gimbal->last_target_timestamp_ms = 0U;
    gimbal->last_position_update_count = X42S_GetPositionUpdateCount(X42S_YAW_MOTOR_ID);
    gimbal->valid_target_count = 0U;
    gimbal->stop_refresh_count = 0U;
    gimbal->filter_ready = false;

#if (GIMBAL_YAW_ONLY_TEST_ENABLED != 0U)
    X42S_Enable(X42S_YAW_MOTOR_ID);
    X42S_Enable(X42S_PITCH_MOTOR_ID);
    X42S_ReadPosition(X42S_YAW_MOTOR_ID);
    gimbal->stopped = false;
#elif (GIMBAL_MOTION_ENABLED == 0U)
    /* A custom gimbal remains electrically disabled until it is calibrated. */
    X42S_Disable(X42S_YAW_MOTOR_ID);
    X42S_Disable(X42S_PITCH_MOTOR_ID);
    gimbal->stopped = true;
#else
    X42S_Enable(X42S_YAW_MOTOR_ID);
    X42S_Enable(X42S_PITCH_MOTOR_ID);
    gimbal->stopped = false;
#endif
}

void Gimbal_Update(Gimbal_Control *gimbal, const MaixCAM_Parser *vision,
                   uint32_t now_ms)
{
    if (gimbal == NULL || vision == NULL) {
        return;
    }

#if (GIMBAL_YAW_ONLY_TEST_ENABLED != 0U)
    MaixCAM_Target target;
#if (GIMBAL_YAW_ONLY_SPEED_MODE != 0U)
    int32_t yaw_speed;
    int32_t speed_delta;
#else
    int32_t yaw_correction;
    int32_t yaw_target;
    int32_t target_delta;
#endif

    if (!MaixCAM_HasValidTarget(vision, now_ms)) {
        gimbal->valid_target_count = 0U;
        gimbal->last_target_timestamp_ms = 0U;
        stop_yaw_tracking(gimbal, now_ms, false);
        return;
    }

    target = MaixCAM_GetTarget(vision);
    if (target.confidence_0p01pct < GIMBAL_YAW_ONLY_CONF_MIN_0P01PCT) {
        gimbal->valid_target_count = 0U;
        gimbal->last_target_timestamp_ms = 0U;
        stop_yaw_tracking(gimbal, now_ms, false);
        return;
    }

    if (target.timestamp_ms != gimbal->last_target_timestamp_ms) {
        gimbal->last_target_timestamp_ms = target.timestamp_ms;
        if (gimbal->valid_target_count < GIMBAL_YAW_ONLY_VALID_FRAMES_MIN) {
            gimbal->valid_target_count++;
        }
    }

    if (gimbal->valid_target_count < GIMBAL_YAW_ONLY_VALID_FRAMES_MIN) {
        stop_yaw_tracking(gimbal, now_ms, false);
        return;
    }

    if ((uint32_t)(now_ms - gimbal->last_update_ms) <
#if (GIMBAL_YAW_ONLY_SPEED_MODE != 0U)
        GIMBAL_YAW_ONLY_SPEED_PERIOD_MS) {
#else
        GIMBAL_YAW_ONLY_PERIOD_MS) {
#endif
        return;
    }

    gimbal->last_update_ms = now_ms;

#if (GIMBAL_YAW_ONLY_SPEED_MODE != 0U)
    if (!update_yaw_only_position_feedback(gimbal, now_ms)) {
        stop_yaw_tracking(gimbal, now_ms, false);
        return;
    }

    yaw_speed = calc_yaw_only_speed_0p1rpm(gimbal,
        filter_yaw_error_0p01deg(gimbal, target.yaw_0p01deg));
    yaw_speed *= GIMBAL_YAW_VISION_TO_MOTOR_SIGN;
    yaw_speed = limit_yaw_only_speed_step(yaw_speed,
                                          gimbal->yaw_speed_0p1rpm);

    if ((yaw_speed > 0 &&
         gimbal->yaw_position_0p1deg >= GIMBAL_YAW_ONLY_MAX_0P1DEG) ||
        (yaw_speed < 0 &&
         gimbal->yaw_position_0p1deg <= GIMBAL_YAW_ONLY_MIN_0P1DEG)) {
        stop_yaw_tracking(gimbal, now_ms, true);
        return;
    }

    speed_delta = yaw_speed - gimbal->yaw_speed_0p1rpm;
    if (speed_delta > -GIMBAL_YAW_ONLY_SPEED_HYST_0P1_RPM &&
        speed_delta < GIMBAL_YAW_ONLY_SPEED_HYST_0P1_RPM) {
        return;
    }

    gimbal->yaw_speed_0p1rpm = yaw_speed;
    if (yaw_speed == 0) {
        stop_yaw_tracking(gimbal, now_ms, true);
        return;
    }

    gimbal->stop_refresh_count = 0U;
    X42S_SetSpeedEx(X42S_YAW_MOTOR_ID, yaw_speed,
                    GIMBAL_YAW_ONLY_ACC_RPM_S, false);
    gimbal->stopped = false;
#else
    yaw_correction = calc_yaw_only_correction_0p1deg(
        filter_yaw_error_0p01deg(gimbal, target.yaw_0p01deg));
    yaw_correction *= GIMBAL_YAW_VISION_TO_MOTOR_SIGN;
    yaw_target = gimbal->yaw_position_0p1deg + yaw_correction;
    yaw_target = clamp_i32(yaw_target, GIMBAL_YAW_ONLY_MIN_0P1DEG,
                           GIMBAL_YAW_ONLY_MAX_0P1DEG);

    target_delta = yaw_target - gimbal->yaw_position_0p1deg;
    if (target_delta > -GIMBAL_YAW_ONLY_TARGET_HYST_0P1DEG &&
        target_delta < GIMBAL_YAW_ONLY_TARGET_HYST_0P1DEG) {
        return;
    }

    gimbal->yaw_position_0p1deg = yaw_target;
    X42S_SetPositionEx(X42S_YAW_MOTOR_ID, yaw_target,
                       GIMBAL_YAW_ONLY_ACC_RPM_S,
                       GIMBAL_YAW_ONLY_ACC_RPM_S,
                       GIMBAL_YAW_ONLY_SPEED_0P1_RPM,
                       0U, false);
    gimbal->stopped = false;
#endif
#elif (GIMBAL_MOTION_ENABLED == 0U)
    (void)now_ms;
    return;
#else
    MaixCAM_Target target;
    int32_t yaw_step;
    int32_t pitch_step;

    if ((uint32_t)(now_ms - gimbal->last_update_ms) <
        GIMBAL_CONTROL_PERIOD_MS) {
        return;
    }

    gimbal->last_update_ms = now_ms;

    if (!MaixCAM_HasValidTarget(vision, now_ms)) {
        Gimbal_Stop(gimbal);
        return;
    }

    target = MaixCAM_GetTarget(vision);

    yaw_step = calc_step_0p1deg(target.yaw_0p01deg,
                                GIMBAL_YAW_KP_NUM,
                                GIMBAL_YAW_KP_DEN);
    pitch_step = calc_step_0p1deg(target.pitch_0p01deg,
                                  GIMBAL_PITCH_KP_NUM,
                                  GIMBAL_PITCH_KP_DEN);
    yaw_step *= GIMBAL_YAW_VISION_TO_MOTOR_SIGN;
    pitch_step *= GIMBAL_PITCH_VISION_TO_MOTOR_SIGN;

    gimbal->yaw_position_0p1deg =
        clamp_i32(gimbal->yaw_position_0p1deg + yaw_step,
                  GIMBAL_YAW_MIN_0P1DEG, GIMBAL_YAW_MAX_0P1DEG);
    gimbal->pitch_position_0p1deg =
        clamp_i32(gimbal->pitch_position_0p1deg + pitch_step,
                  GIMBAL_PITCH_MIN_0P1DEG, GIMBAL_PITCH_MAX_0P1DEG);

    X42S_SetPosition(X42S_YAW_MOTOR_ID, gimbal->yaw_position_0p1deg);
    X42S_SetPosition(X42S_PITCH_MOTOR_ID, gimbal->pitch_position_0p1deg);
    gimbal->stopped = false;
#endif
}

void Gimbal_Stop(Gimbal_Control *gimbal)
{
    if (gimbal != NULL && gimbal->stopped) {
        return;
    }

    X42S_Stop(X42S_YAW_MOTOR_ID);
    X42S_Stop(X42S_PITCH_MOTOR_ID);

    if (gimbal != NULL) {
        gimbal->stopped = true;
    }
}
