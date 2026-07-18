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
    uint32_t last_update_ms;
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
