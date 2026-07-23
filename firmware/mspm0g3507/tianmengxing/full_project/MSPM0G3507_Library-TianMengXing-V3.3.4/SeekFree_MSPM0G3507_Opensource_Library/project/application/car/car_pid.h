#ifndef CAR_PID_H
#define CAR_PID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t target;
    int32_t feedback;
    int32_t error;
    int32_t last_error;
    int32_t prev_error;
    int32_t integral;
    float kp;
    float ki;
    float kd;
    int32_t output_min;
    int32_t output_max;
    int32_t integral_min;
    int32_t integral_max;
    float output_float;
    int32_t output;
} CarPid;

void CarPid_Init(CarPid *pid, float kp, float ki, float kd,
                 int32_t output_min, int32_t output_max,
                 int32_t integral_min, int32_t integral_max);
void CarPid_Reset(CarPid *pid);
int32_t CarPid_UpdateIncremental(CarPid *pid, int32_t target, int32_t feedback);

#ifdef __cplusplus
}
#endif

#endif
