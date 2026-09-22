#ifndef H_BALANCE_CONTROL_H
#define H_BALANCE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "../config/h_balance_config.h"
#include "../vision/maixcam_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    H_BALANCE_STATE_DISABLED = 0,
    H_BALANCE_STATE_WAIT_MOTOR,
    H_BALANCE_STATE_WAIT_VISION,
    H_BALANCE_STATE_ACTIVE,
    H_BALANCE_STATE_SETTLED,
    H_BALANCE_STATE_VISION_LOST,
    H_BALANCE_STATE_FAULT
} HBalance_State;

typedef enum
{
    H_BALANCE_FAULT_NONE = 0,
    H_BALANCE_FAULT_ARM_TIMEOUT,
    H_BALANCE_FAULT_MOTOR_FEEDBACK_TIMEOUT,
    H_BALANCE_FAULT_MOTOR_RANGE,
    H_BALANCE_FAULT_INVALID_TARGET
} HBalance_Fault;

typedef struct
{
    HBalance_State state;
    HBalance_Fault fault;
    int32_t target_0p01cm;
    int32_t estimated_position_0p01cm;
    int32_t estimated_velocity_0p01cm_s;
    int32_t requested_tilt_0p1deg;
    int32_t commanded_position_0p1deg;
    int32_t level_zero_0p1deg;
    int32_t motor_position_0p1deg;
    int64_t integral_error_0p01cm_ms;
    uint32_t last_update_ms;
    uint32_t last_valid_ball_ms;
    uint32_t last_motor_feedback_ms;
    uint32_t last_motor_request_ms;
    uint32_t last_motor_command_ms;
    uint32_t last_bus_tx_ms;
    uint32_t arm_started_ms;
    uint32_t settle_started_ms;
    uint32_t last_capture_ms;
    uint32_t target_timestamp_ms;
    uint32_t key_raw_since_ms;
    uint32_t key_released_since_ms;
    uint16_t last_ball_seq;
    uint16_t motor_update_count;
    uint8_t valid_frame_count;
    bool armed;
    bool estimator_ready;
    bool has_ball_sample;
    bool key_raw_pressed;
    bool key_stable_pressed;
    bool key_release_seen;
    bool key_armed;
} HBalance_Control;

void HBalance_Init(HBalance_Control *control);
void HBalance_SetArmKey(HBalance_Control *control, bool pressed,
                        uint32_t now_ms);
void HBalance_RequestArm(HBalance_Control *control, uint32_t now_ms);
void HBalance_Disarm(HBalance_Control *control);
bool HBalance_SetTarget(HBalance_Control *control, int32_t target_0p01cm);
void HBalance_Update(HBalance_Control *control,
                     const MaixCAM_Parser *vision, uint32_t now_ms);

/* Pure control-law entry used by host simulation and tuning tools. */
int32_t HBalance_ComputeTilt0p1Deg(int32_t target_0p01cm,
                                   int32_t position_0p01cm,
                                   int32_t velocity_0p01cm_s,
                                   int32_t integral_acc_0p01cm_s2);

#ifdef __cplusplus
}
#endif

#endif
