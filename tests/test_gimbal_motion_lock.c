#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../application/gimbal/gimbal_control.h"
#include "../application/motor/x42s_rs485/x42s_rs485.h"
#include "../application/vision/maixcam_protocol.h"

static uint8_t g_last_frame[16];
static size_t g_last_len;
static uint32_t g_send_count;

static void mock_send(const uint8_t *data, size_t len)
{
    assert(len <= sizeof(g_last_frame));
    memcpy(g_last_frame, data, len);
    g_last_len = len;
    ++g_send_count;
}

static void mock_set_tx_enable(bool enable)
{
    (void)enable;
}

static void feed_target(MaixCAM_Parser *parser)
{
    const uint8_t target_frame[] = {
        0xAAU, 0x55U, 0x01U, 0x06U, 0xC8U, 0x00U,
        0x9CU, 0xFFU, 0x7AU, 0x26U, 0x0AU
    };
    size_t index;

    for (index = 0U; index < sizeof(target_frame); ++index) {
        (void)MaixCAM_ProtocolInputByte(parser, target_frame[index], 20U);
    }
}

int main(void)
{
    static const X42S_PortOps ops = {mock_send, mock_set_tx_enable};
    const uint8_t yaw_disable[] = {0x02U, 0xF3U, 0xABU, 0x00U, 0x00U, 0x6BU};
    const uint8_t pitch_disable[] = {0x01U, 0xF3U, 0xABU, 0x00U, 0x00U, 0x6BU};
    MaixCAM_Parser parser;
    Gimbal_Control gimbal;

    X42S_SetPortOps(&ops);
    MaixCAM_ProtocolInit(&parser);
    feed_target(&parser);
    Gimbal_Init(&gimbal);

    assert(gimbal.stopped);
    assert(g_send_count == 2U);
    assert(g_last_len == sizeof(pitch_disable));
    assert(memcmp(g_last_frame, pitch_disable, sizeof(pitch_disable)) == 0);

    Gimbal_Update(&gimbal, &parser, 20U);
    assert(g_send_count == 2U);
    assert(gimbal.yaw_position_0p1deg == 0);
    assert(gimbal.pitch_position_0p1deg == 0);

    X42S_Disable(X42S_YAW_MOTOR_ID);
    assert(memcmp(g_last_frame, yaw_disable, sizeof(yaw_disable)) == 0);
    return 0;
}
