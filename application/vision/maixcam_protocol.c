#include "maixcam_protocol.h"

#include <string.h>

enum
{
    MAIXCAM_STATE_WAIT_HEADER_0 = 0,
    MAIXCAM_STATE_WAIT_HEADER_1,
    MAIXCAM_STATE_CMD,
    MAIXCAM_STATE_LEN,
    MAIXCAM_STATE_DATA,
    MAIXCAM_STATE_CHECK
};

static int16_t read_i16_le(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static bool payload_len_is_valid(uint8_t cmd, uint8_t len)
{
    switch (cmd) {
    case MAIXCAM_CMD_TARGET_FOUND:
        return len == 6U;
    case MAIXCAM_CMD_TARGET_LOST:
        return len == 0U;
    case MAIXCAM_CMD_HEARTBEAT:
        return len == 6U;
    case MAIXCAM_CMD_ERROR:
        return len == 1U;
    case MAIXCAM_CMD_BALL_STATE:
        return len == 16U;
    case MAIXCAM_CMD_BALL_TARGET_SELECT:
        return len == 2U;
    default:
        return false;
    }
}

static void reset_frame(MaixCAM_Parser *parser, uint8_t state)
{
    parser->state = state;
    parser->cmd = 0U;
    parser->len = 0U;
    parser->index = 0U;
    parser->checksum = 0U;
}

static bool apply_frame(MaixCAM_Parser *parser, uint32_t now_ms)
{
    parser->last_frame_ms = now_ms;
    parser->last_command = parser->cmd;

    switch (parser->cmd) {
    case MAIXCAM_CMD_TARGET_FOUND:
        parser->target.valid = true;
        parser->target.yaw_0p01deg = read_i16_le(&parser->payload[0]);
        parser->target.pitch_0p01deg = read_i16_le(&parser->payload[2]);
        parser->target.confidence_0p01pct = read_u16_le(&parser->payload[4]);
        parser->target.timestamp_ms = now_ms;
        parser->last_valid_target_ms = now_ms;
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    case MAIXCAM_CMD_TARGET_LOST:
        parser->target.valid = false;
        parser->target.yaw_0p01deg = 0;
        parser->target.pitch_0p01deg = 0;
        parser->target.confidence_0p01pct = 0U;
        parser->target.timestamp_ms = now_ms;
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    case MAIXCAM_CMD_HEARTBEAT:
        parser->heartbeat.uptime_ms = read_u32_le(&parser->payload[0]);
        parser->heartbeat.seq = read_u16_le(&parser->payload[4]);
        parser->heartbeat.timestamp_ms = now_ms;
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    case MAIXCAM_CMD_ERROR:
        parser->remote_error_code = parser->payload[0];
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    case MAIXCAM_CMD_BALL_STATE:
        parser->ball.seq = read_u16_le(&parser->payload[0]);
        parser->ball.capture_ms = read_u32_le(&parser->payload[2]);
        parser->ball.position_0p01cm = read_i16_le(&parser->payload[6]);
        parser->ball.velocity_0p01cm_s = read_i16_le(&parser->payload[8]);
        parser->ball.confidence_0p01pct = read_u16_le(&parser->payload[10]);
        parser->ball.processing_ms = read_u16_le(&parser->payload[12]);
        parser->ball.flags = parser->payload[14];
        parser->ball.source = parser->payload[15];
        parser->ball.valid =
            (parser->ball.flags & MAIXCAM_BALL_FLAG_VALID) != 0U;
        parser->ball.velocity_valid =
            (parser->ball.flags & MAIXCAM_BALL_FLAG_VELOCITY_VALID) != 0U;
        parser->ball.pipe_locked =
            (parser->ball.flags & MAIXCAM_BALL_FLAG_PIPE_LOCKED) != 0U;
        parser->ball.predicted =
            (parser->ball.flags & MAIXCAM_BALL_FLAG_PREDICTED) != 0U;
        parser->ball.timestamp_ms = now_ms;
        if (parser->ball.valid) {
            parser->last_valid_target_ms = now_ms;
        }
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    case MAIXCAM_CMD_BALL_TARGET_SELECT:
        parser->target_position.position_0p01cm =
            read_i16_le(&parser->payload[0]);
        parser->target_position.timestamp_ms = now_ms;
        parser->last_error = MAIXCAM_ERROR_NONE;
        return true;

    default:
        parser->last_error = MAIXCAM_ERROR_UNKNOWN_CMD;
        return false;
    }
}

void MaixCAM_ProtocolInit(MaixCAM_Parser *parser)
{
    if (parser == NULL) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    parser->state = MAIXCAM_STATE_WAIT_HEADER_0;
}

bool MaixCAM_ProtocolInputByte(MaixCAM_Parser *parser, uint8_t byte,
                               uint32_t now_ms)
{
    if (parser == NULL) {
        return false;
    }

    switch (parser->state) {
    case MAIXCAM_STATE_WAIT_HEADER_0:
        if (byte == MAIXCAM_FRAME_HEADER_0) {
            reset_frame(parser, MAIXCAM_STATE_WAIT_HEADER_1);
        }
        return false;

    case MAIXCAM_STATE_WAIT_HEADER_1:
        if (byte == MAIXCAM_FRAME_HEADER_1) {
            parser->state = MAIXCAM_STATE_CMD;
        } else if (byte != MAIXCAM_FRAME_HEADER_0) {
            reset_frame(parser, MAIXCAM_STATE_WAIT_HEADER_0);
        }
        return false;

    case MAIXCAM_STATE_CMD:
        parser->cmd = byte;
        parser->checksum = byte;
        parser->state = MAIXCAM_STATE_LEN;
        return false;

    case MAIXCAM_STATE_LEN:
        parser->len = byte;
        parser->checksum = (uint8_t)(parser->checksum + byte);
        parser->index = 0U;

        if (parser->len > MAIXCAM_MAX_PAYLOAD_LEN ||
            !payload_len_is_valid(parser->cmd, parser->len)) {
            parser->last_error = MAIXCAM_ERROR_LENGTH;
            reset_frame(parser, MAIXCAM_STATE_WAIT_HEADER_0);
            return false;
        }

        parser->state = (parser->len == 0U) ? MAIXCAM_STATE_CHECK :
                         MAIXCAM_STATE_DATA;
        return false;

    case MAIXCAM_STATE_DATA:
        parser->payload[parser->index++] = byte;
        parser->checksum = (uint8_t)(parser->checksum + byte);
        if (parser->index >= parser->len) {
            parser->state = MAIXCAM_STATE_CHECK;
        }
        return false;

    case MAIXCAM_STATE_CHECK:
        if (byte == parser->checksum) {
            bool accepted = apply_frame(parser, now_ms);
            reset_frame(parser, MAIXCAM_STATE_WAIT_HEADER_0);
            return accepted;
        }

        parser->last_error = MAIXCAM_ERROR_CHECKSUM;
        reset_frame(parser, (byte == MAIXCAM_FRAME_HEADER_0) ?
                    MAIXCAM_STATE_WAIT_HEADER_1 :
                    MAIXCAM_STATE_WAIT_HEADER_0);
        return false;

    default:
        reset_frame(parser, MAIXCAM_STATE_WAIT_HEADER_0);
        return false;
    }
}

bool MaixCAM_IsTimeout(const MaixCAM_Parser *parser, uint32_t now_ms)
{
    if (parser == NULL || parser->last_frame_ms == 0U) {
        return true;
    }

    return (uint32_t)(now_ms - parser->last_frame_ms) > MAIXCAM_TIMEOUT_MS;
}

bool MaixCAM_HasValidTarget(const MaixCAM_Parser *parser, uint32_t now_ms)
{
    if (parser == NULL || !parser->target.valid) {
        return false;
    }

    return (uint32_t)(now_ms - parser->last_valid_target_ms) <=
           MAIXCAM_TIMEOUT_MS;
}

MaixCAM_Target MaixCAM_GetTarget(const MaixCAM_Parser *parser)
{
    MaixCAM_Target empty = {0};

    if (parser == NULL) {
        return empty;
    }

    return parser->target;
}

bool MaixCAM_HasValidBall(const MaixCAM_Parser *parser, uint32_t now_ms)
{
    if (parser == NULL || !parser->ball.valid) {
        return false;
    }
    return (uint32_t)(now_ms - parser->ball.timestamp_ms) <=
           MAIXCAM_TIMEOUT_MS;
}

MaixCAM_BallState MaixCAM_GetBall(const MaixCAM_Parser *parser)
{
    MaixCAM_BallState empty = {0};

    if (parser == NULL) {
        return empty;
    }
    return parser->ball;
}
