#include "car_pid.h"

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

static float limit_float(float value, float min_value, float max_value)
{
    if (value > max_value) {
        return max_value;
    }
    if (value < min_value) {
        return min_value;
    }
    return value;
}

void CarPid_Init(CarPid *pid, float kp, float ki, float kd,
                 int32_t output_min, int32_t output_max,
                 int32_t integral_min, int32_t integral_max)
{
    if (pid == 0) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    CarPid_Reset(pid);
}

void CarPid_Reset(CarPid *pid)
{
    if (pid == 0) {
        return;
    }

    pid->target = 0;
    pid->feedback = 0;
    pid->error = 0;
    pid->last_error = 0;
    pid->prev_error = 0;
    pid->integral = 0;
    pid->output_float = 0.0f;
    pid->output = 0;
}

int32_t CarPid_UpdateIncremental(CarPid *pid, int32_t target, int32_t feedback)
{
    float delta_output;

    if (pid == 0) {
        return 0;
    }

    pid->target = target;
    pid->feedback = feedback;
    pid->error = pid->target - pid->feedback;
    pid->integral += pid->error;
    pid->integral = limit_i32(pid->integral, pid->integral_min,
                              pid->integral_max);

    delta_output =
        (pid->kp * (float)(pid->error - pid->last_error)) +
        (pid->ki * (float)pid->error) +
        (pid->kd * (float)(pid->error - (2 * pid->last_error) + pid->prev_error));

    pid->output_float += delta_output;
    pid->output_float = limit_float(pid->output_float,
                                    (float)pid->output_min,
                                    (float)pid->output_max);
    pid->output = (int32_t)pid->output_float;
    pid->prev_error = pid->last_error;
    pid->last_error = pid->error;

    return pid->output;
}
