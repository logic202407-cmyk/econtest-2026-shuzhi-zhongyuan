#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../application/gimbal/gimbal_control.h"
#include "../application/motor/x42s_rs485/x42s_rs485.h"
#include "../application/vision/maixcam_protocol.h"

static uint8_t g_last_frame[32];
static size_t g_last_len;
static uint32_t g_send_count;
static uint32_t g_tx_enable_count;
static uint32_t g_tx_disable_count;

static void mock_send(const uint8_t *data, size_t len)
{
    assert(len <= sizeof(g_last_frame));
    memcpy(g_last_frame, data, len);
    g_last_len = len;
    ++g_send_count;
}

static void mock_set_tx_enable(bool enable)
{
    if (enable) {
        ++g_tx_enable_count;
    } else {
        ++g_tx_disable_count;
    }
}

static bool feed(MaixCAM_Parser *parser, const uint8_t *data, size_t len,
                 uint32_t now_ms)
{
    bool accepted = false;
    size_t index;

    for (index = 0U; index < len; ++index) {
        accepted = MaixCAM_ProtocolInputByte(parser, data[index], now_ms) || accepted;
    }
    return accepted;
}

static void reset_motor_mock(void)
{
    memset(g_last_frame, 0, sizeof(g_last_frame));
    g_last_len = 0U;
    g_send_count = 0U;
    g_tx_enable_count = 0U;
    g_tx_disable_count = 0U;
}

static void test_maixcam_parser(void)
{
    MaixCAM_Parser parser;
    const uint8_t target_frame[] = {
        0xAAU, 0x55U, 0x01U, 0x06U, 0x7BU, 0x00U,
        0xD3U, 0xFFU, 0x7AU, 0x26U, 0xF4U
    };
    const uint8_t lost_frame[] = {0xAAU, 0x55U, 0x02U, 0x00U, 0x02U};
    uint8_t bad_frame[sizeof(target_frame)];

    MaixCAM_ProtocolInit(&parser);
    assert(feed(&parser, target_frame, sizeof(target_frame), 100U));
    assert(parser.target.valid);
    assert(parser.target.yaw_0p01deg == 123);
    assert(parser.target.pitch_0p01deg == -45);
    assert(parser.target.confidence_0p01pct == 9850U);
    assert(MaixCAM_HasValidTarget(&parser, 600U));
    assert(!MaixCAM_HasValidTarget(&parser, 601U));

    memcpy(bad_frame, target_frame, sizeof(bad_frame));
    bad_frame[sizeof(bad_frame) - 1U] ^= 0x01U;
    MaixCAM_ProtocolInit(&parser);
    assert(!feed(&parser, bad_frame, sizeof(bad_frame), 100U));
    assert(parser.last_error == MAIXCAM_ERROR_CHECKSUM);

    MaixCAM_ProtocolInit(&parser);
    assert(!feed(&parser, target_frame, 5U, 100U));
    assert(!feed(&parser, target_frame, sizeof(target_frame), 101U));
    assert(feed(&parser, target_frame, sizeof(target_frame), 102U));
    assert(feed(&parser, lost_frame, sizeof(lost_frame), 120U));
    assert(!parser.target.valid);
}

static void test_x42s_frames(void)
{
    static const X42S_PortOps ops = {mock_send, mock_set_tx_enable};
    const uint8_t enable_expected[] = {0x01U, 0xF3U, 0xABU, 0x01U, 0x00U, 0x6BU};
    const uint8_t stop_expected[] = {0x01U, 0xFEU, 0x98U, 0x00U, 0x6BU};
    const uint8_t position_expected[] = {
        0x01U, 0xFDU, 0x01U, 0x00U, 0x64U, 0x00U, 0x64U, 0x01U,
        0x2CU, 0x00U, 0x00U, 0x00U, 0x32U, 0x02U, 0x00U, 0x6BU
    };

    reset_motor_mock();
    X42S_SetPortOps(&ops);

    X42S_Enable(1U);
    assert(g_last_len == sizeof(enable_expected));
    assert(memcmp(g_last_frame, enable_expected, sizeof(enable_expected)) == 0);

    X42S_SetPosition(1U, -50);
    assert(g_last_len == sizeof(position_expected));
    assert(memcmp(g_last_frame, position_expected, sizeof(position_expected)) == 0);

    X42S_Stop(1U);
    assert(g_last_len == sizeof(stop_expected));
    assert(memcmp(g_last_frame, stop_expected, sizeof(stop_expected)) == 0);
    assert(g_tx_enable_count == g_tx_disable_count);
    assert(g_tx_enable_count == 3U);
}

static void test_gimbal_timeout_stop(void)
{
    MaixCAM_Parser parser;
    Gimbal_Control gimbal;
    const uint8_t target_frame[] = {
        0xAAU, 0x55U, 0x01U, 0x06U, 0xC8U, 0x00U,
        0x9CU, 0xFFU, 0x7AU, 0x26U, 0x0AU
    };

    reset_motor_mock();
    MaixCAM_ProtocolInit(&parser);
    assert(feed(&parser, target_frame, sizeof(target_frame), 20U));
    Gimbal_Init(&gimbal);
    assert(g_send_count == 2U);

    Gimbal_Update(&gimbal, &parser, 20U);
    assert(!gimbal.stopped);
    assert(gimbal.yaw_position_0p1deg == 2);
    assert(gimbal.pitch_position_0p1deg == -1);

    Gimbal_Update(&gimbal, &parser, 521U);
    assert(gimbal.stopped);
    assert(g_send_count == 6U);
}

int main(void)
{
    test_maixcam_parser();
    test_x42s_frames();
    test_gimbal_timeout_stop();
    return 0;
}
