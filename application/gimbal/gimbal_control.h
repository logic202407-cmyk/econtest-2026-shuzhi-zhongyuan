#ifndef GIMBAL_CONTROL_H
#define GIMBAL_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "../config/app_config.h"
#include "../vision/maixcam_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t yaw_position_0p1deg;
    int32_t pitch_position_0p1deg;
    int32_t filtered_yaw_0p01deg;
    int32_t previous_yaw_error_0p01deg;
    int32_t yaw_speed_0p1rpm;
    uint32_t last_update_ms;
    uint32_t last_motion_ms;
    uint32_t last_position_request_ms;
    uint32_t last_position_feedback_ms;
    uint32_t last_stop_ms;
    uint16_t last_position_update_count;
    uint8_t stop_refresh_count;
    bool filter_ready;
    bool stopped;
} Gimbal_Control;

void Gimbal_Init(Gimbal_Control *gimbal);
void Gimbal_Update(Gimbal_Control *gimbal, const MaixCAM_Parser *vision,
                   uint32_t now_ms);
void Gimbal_Stop(Gimbal_Control *gimbal);

#ifdef __cplusplus
}
#endif

#endif
