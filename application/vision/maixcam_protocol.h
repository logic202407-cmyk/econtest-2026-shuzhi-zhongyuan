#ifndef MAIXCAM_PROTOCOL_H
#define MAIXCAM_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "../config/app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MAIXCAM_CMD_TARGET_FOUND = 0x01U,
    MAIXCAM_CMD_TARGET_LOST = 0x02U,
    MAIXCAM_CMD_HEARTBEAT = 0x03U,
    MAIXCAM_CMD_ERROR = 0x04U
} MaixCAM_Command;

typedef enum
{
    MAIXCAM_ERROR_NONE = 0,
    MAIXCAM_ERROR_CHECKSUM = 1,
    MAIXCAM_ERROR_LENGTH = 2,
    MAIXCAM_ERROR_UNKNOWN_CMD = 3
} MaixCAM_Error;

typedef struct
{
    bool valid;
    int16_t yaw_0p01deg;
    int16_t pitch_0p01deg;
    uint16_t confidence_0p01pct;
    uint32_t timestamp_ms;
} MaixCAM_Target;

typedef struct
{
    uint32_t uptime_ms;
    uint16_t seq;
    uint32_t timestamp_ms;
} MaixCAM_Heartbeat;

typedef struct
{
    MaixCAM_Target target;
    MaixCAM_Heartbeat heartbeat;
    MaixCAM_Error last_error;
    uint8_t remote_error_code;
    uint32_t last_frame_ms;
    uint32_t last_valid_target_ms;

    uint8_t state;
    uint8_t cmd;
    uint8_t len;
    uint8_t index;
    uint8_t checksum;
    uint8_t payload[MAIXCAM_MAX_PAYLOAD_LEN];
} MaixCAM_Parser;

void MaixCAM_ProtocolInit(MaixCAM_Parser *parser);
bool MaixCAM_ProtocolInputByte(MaixCAM_Parser *parser, uint8_t byte,
                               uint32_t now_ms);
bool MaixCAM_IsTimeout(const MaixCAM_Parser *parser, uint32_t now_ms);
bool MaixCAM_HasValidTarget(const MaixCAM_Parser *parser, uint32_t now_ms);
MaixCAM_Target MaixCAM_GetTarget(const MaixCAM_Parser *parser);

#ifdef __cplusplus
}
#endif

#endif
