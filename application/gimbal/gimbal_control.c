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

static int32_t limit_yaw_only_step(int32_t step)
{
    return clamp_i32(step, -GIMBAL_YAW_ONLY_STEP_LIMIT_0P1DEG,
                     GIMBAL_YAW_ONLY_STEP_LIMIT_0P1DEG);
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

static int32_t calc_yaw_only_step_0p1deg(int16_t error_0p01deg)
{
    int32_t motor_error_0p1deg;
    int32_t step_0p1deg;

    if (error_0p01deg > -GIMBAL_YAW_ONLY_DEADBAND_0P01DEG &&
        error_0p01deg < GIMBAL_YAW_ONLY_DEADBAND_0P01DEG) {
        return 0;
    }

    if (GIMBAL_YAW_ONLY_KP_DEN == 0) {
        return 0;
    }

    motor_error_0p1deg = ((int32_t)error_0p01deg * 10) /
                         (int32_t)VISION_ANGLE_UNITS_PER_DEG;
    step_0p1deg = (motor_error_0p1deg * GIMBAL_YAW_ONLY_KP_NUM) /
                  GIMBAL_YAW_ONLY_KP_DEN;
    return limit_yaw_only_step(step_0p1deg);
}
#endif

void Gimbal_Init(Gimbal_Control *gimbal)
{
    if (gimbal == NULL) {
        return;
    }

    gimbal->yaw_position_0p1deg = 0;
    gimbal->pitch_position_0p1deg = 0;
    gimbal->last_update_ms = 0U;

#if (GIMBAL_YAW_ONLY_TEST_ENABLED != 0U)
    X42S_Enable(X42S_YAW_MOTOR_ID);
    X42S_Enable(X42S_PITCH_MOTOR_ID);
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
    int32_t yaw_step;

    if ((uint32_t)(now_ms - gimbal->last_update_ms) <
        GIMBAL_CONTROL_PERIOD_MS) {
        return;
    }

    gimbal->last_update_ms = now_ms;

    if (!MaixCAM_HasValidTarget(vision, now_ms)) {
        X42S_Stop(X42S_YAW_MOTOR_ID);
        gimbal->stopped = true;
        return;
    }

    target = MaixCAM_GetTarget(vision);
    if (target.confidence_0p01pct < GIMBAL_YAW_ONLY_CONF_MIN_0P01PCT) {
        X42S_Stop(X42S_YAW_MOTOR_ID);
        gimbal->stopped = true;
        return;
    }

    yaw_step = calc_yaw_only_step_0p1deg(target.yaw_0p01deg);
    yaw_step *= GIMBAL_YAW_VISION_TO_MOTOR_SIGN;

    if (yaw_step == 0) {
        return;
    }

    if ((yaw_step > 0 &&
         gimbal->yaw_position_0p1deg >= GIMBAL_YAW_ONLY_MAX_0P1DEG) ||
        (yaw_step < 0 &&
         gimbal->yaw_position_0p1deg <= GIMBAL_YAW_ONLY_MIN_0P1DEG)) {
        X42S_Stop(X42S_YAW_MOTOR_ID);
        gimbal->stopped = true;
        return;
    }

    yaw_step = clamp_i32(yaw_step,
                         GIMBAL_YAW_ONLY_MIN_0P1DEG - gimbal->yaw_position_0p1deg,
                         GIMBAL_YAW_ONLY_MAX_0P1DEG - gimbal->yaw_position_0p1deg);
    gimbal->yaw_position_0p1deg += yaw_step;
    X42S_NudgeRelative(X42S_YAW_MOTOR_ID, yaw_step);
    gimbal->stopped = false;
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
