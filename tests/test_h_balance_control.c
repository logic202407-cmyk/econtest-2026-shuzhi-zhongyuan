#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../application/balance/h_balance_control.h"
#include "../application/motor/x42s_rs485/x42s_rs485.h"
#include "../application/vision/maixcam_protocol.h"

static uint8_t g_last_tx[32];
static size_t g_last_tx_len;
static uint32_t g_tx_count;

static void capture_send(const uint8_t *data, size_t len)
{
    assert(len <= sizeof(g_last_tx));
    memcpy(g_last_tx, data, len);
    g_last_tx_len = len;
    g_tx_count++;
}

static void capture_direction(bool enable)
{
    (void)enable;
}

static void feed_motor_position_zero(void)
{
    static const uint8_t reply[] = {
        H_BALANCE_MOTOR_ID, 0x36U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, X42S_CHECK_FIXED
    };
    size_t i;

    for (i = 0U; i < sizeof(reply); ++i) {
        X42S_OnRxByte(reply[i]);
    }
}

static void put_i16_le(uint8_t *data, int16_t value)
{
    data[0] = (uint8_t)((uint16_t)value & 0xFFU);
    data[1] = (uint8_t)((uint16_t)value >> 8);
}

static void put_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void feed_ball_frame(MaixCAM_Parser *parser, uint16_t seq,
                            uint32_t capture_ms, int16_t position_0p01cm,
                            int16_t velocity_0p01cm_s, uint32_t now_ms)
{
    uint8_t frame[21] = {0};
    uint8_t checksum = 0U;
    size_t i;

    frame[0] = MAIXCAM_FRAME_HEADER_0;
    frame[1] = MAIXCAM_FRAME_HEADER_1;
    frame[2] = MAIXCAM_CMD_BALL_STATE;
    frame[3] = 16U;
    put_u16_le(&frame[4], seq);
    put_u32_le(&frame[6], capture_ms);
    put_i16_le(&frame[10], position_0p01cm);
    put_i16_le(&frame[12], velocity_0p01cm_s);
    put_u16_le(&frame[14], 9000U);
    put_u16_le(&frame[16], 8U);
    frame[18] = MAIXCAM_BALL_FLAG_VALID |
                MAIXCAM_BALL_FLAG_VELOCITY_VALID |
                MAIXCAM_BALL_FLAG_PIPE_LOCKED;
    frame[19] = MAIXCAM_BALL_SOURCE_YOLO;
    for (i = 2U; i < 20U; ++i) {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    frame[20] = checksum;

    for (i = 0U; i < sizeof(frame); ++i) {
        (void)MaixCAM_ProtocolInputByte(parser, frame[i], now_ms);
    }
}

static void test_control_law_sign_and_braking(void)
{
    int32_t tilt;

    /* Ball is too close to the lift end: CW/positive must raise it and
     * accelerate the ball toward the fixed end. */
    tilt = HBalance_ComputeTilt0p1Deg(1250, 500, 0, 0);
    assert(tilt > 0);

    /* Ball is past the target: CCW/negative lowers the lift end. */
    tilt = HBalance_ComputeTilt0p1Deg(1250, 2000, 0, 0);
    assert(tilt < 0);

    /* A fast ball approaching the target must command reverse tilt to brake
     * before it crosses the target. */
    tilt = HBalance_ComputeTilt0p1Deg(1250, 1100, 2000, 0);
    assert(tilt < 0);

    assert(HBalance_ComputeTilt0p1Deg(1250, 50, 0, 0) > 0);
    assert(HBalance_ComputeTilt0p1Deg(1250, 2450, 0, 0) < 0);
}

static void test_protocol_and_closed_loop_arm(void)
{
    static const X42S_PortOps port = {capture_send, capture_direction};
    HBalance_Control control;
    MaixCAM_Parser parser;

    X42S_SetPortOps(&port);
    MaixCAM_ProtocolInit(&parser);
    HBalance_Init(&control);

    HBalance_RequestArm(&control, 100U);
    assert(control.state == H_BALANCE_STATE_WAIT_MOTOR);
    assert(g_last_tx_len == 3U);
    assert(g_last_tx[0] == H_BALANCE_MOTOR_ID);
    assert(g_last_tx[1] == 0x36U);

    feed_motor_position_zero();
    HBalance_Update(&control, &parser, 110U);
    assert(control.armed);
    assert(control.state == H_BALANCE_STATE_WAIT_VISION);
    assert(g_last_tx[1] == 0xF3U);

    feed_ball_frame(&parser, 1U, 120U, 500, 0, 128U);
    HBalance_Update(&control, &parser, 128U);
    feed_motor_position_zero();
    feed_ball_frame(&parser, 2U, 150U, 505, 15, 158U);
    HBalance_Update(&control, &parser, 158U);
    feed_motor_position_zero();
    feed_ball_frame(&parser, 3U, 180U, 510, 15, 188U);
    HBalance_Update(&control, &parser, 188U);

    assert(parser.ball.valid);
    assert(parser.ball.pipe_locked);
    assert(control.state == H_BALANCE_STATE_ACTIVE);
    assert(control.requested_tilt_0p1deg > 0);
    assert(g_last_tx_len == 13U);
    assert(g_last_tx[1] == 0xFDU);
    assert(g_last_tx[2] == X42S_DIR_CW);

    /* Keeping motor feedback fresh while vision expires must return the rail
     * toward level instead of continuing a blind acceleration command. */
    feed_motor_position_zero();
    HBalance_Update(&control, &parser, 500U);
    feed_motor_position_zero();
    HBalance_Update(&control, &parser, 530U);
    assert(control.state == H_BALANCE_STATE_VISION_LOST);
    assert(control.requested_tilt_0p1deg == 0);
}

int main(void)
{
    test_control_law_sign_and_braking();
    test_protocol_and_closed_loop_arm();
    printf("H-balance control tests passed (%lu motor frames captured).\n",
           (unsigned long)g_tx_count);
    return 0;
}
