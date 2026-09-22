#include "x42s_rs485.h"

#if X42S_FIRMWARE_EMM_FREE != 1U
#error "The current x42s_rs485 driver supports only Emm firmware free protocol."
#endif

#ifndef X42S_MAX_ID
#define X42S_MAX_ID 32U
#endif

#if defined(__CC_ARM) && !defined(__clang__)
#define X42S_WEAK __weak
#elif defined(_MSC_VER)
#define X42S_WEAK
#else
#define X42S_WEAK __attribute__((weak))
#endif

static int32_t g_last_position[X42S_MAX_ID + 1U];
static int32_t g_last_speed[X42S_MAX_ID + 1U];
static uint16_t g_position_update_count[X42S_MAX_ID + 1U];
static uint8_t g_rx_buf[16];
static uint8_t g_rx_len;
static uint8_t g_rx_expected_len;
static uint8_t g_last_reply_id;
static uint8_t g_last_reply_command;
static X42S_PortOps g_port_ops;

static uint32_t abs_i32(int32_t value)
{
    if (value < 0) {
        return (uint32_t)(-(value + 1)) + 1U;
    }

    return (uint32_t)value;
}

static uint16_t speed_0p1rpm_to_rpm(uint16_t speed_0p1rpm)
{
    uint32_t rpm = ((uint32_t)speed_0p1rpm + 5U) / 10U;

    if (rpm > 0xFFFFU) {
        return 0xFFFFU;
    }

    return (uint16_t)rpm;
}

static uint8_t acc_rpm_s_to_emm(uint16_t acc_rpm_s)
{
    uint32_t acc = ((uint32_t)acc_rpm_s + 5U) / 10U;

    if (acc > 0xFFU) {
        return 0xFFU;
    }

    return (uint8_t)acc;
}

static uint32_t angle_0p1deg_to_pulses(int32_t angle_0p1deg)
{
    uint32_t angle = abs_i32(angle_0p1deg);
    uint32_t pulses = (angle * X42S_EMM_POSITION_PULSES_PER_REV + 1800U) / 3600U;

    if (angle != 0U && pulses == 0U) {
        pulses = 1U;
    }

    return pulses;
}

static int32_t encoder_counts_to_angle_0p1deg(uint32_t counts, uint8_t sign)
{
    int32_t angle = (int32_t)((counts * 3600U + (X42S_EMM_ENCODER_COUNTS_PER_REV / 2U)) /
                             X42S_EMM_ENCODER_COUNTS_PER_REV);

    return sign ? -angle : angle;
}

static void put_u16(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)value;
}

static void put_u32(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24);
    buf[1] = (uint8_t)(value >> 16);
    buf[2] = (uint8_t)(value >> 8);
    buf[3] = (uint8_t)value;
}

static uint16_t get_u16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

static uint32_t get_u32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           (uint32_t)buf[3];
}

static uint8_t reply_length_for_command(uint8_t command)
{
    switch (command) {
    case 0x35U:
        return 6U;
    case 0x36U:
        return 8U;
    case 0xF3U:
    case 0xF6U:
    case 0xFDU:
    case 0xFEU:
        return 4U;
    default:
        return 0U;
    }
}

static void reset_rx(void)
{
    g_rx_len = 0U;
    g_rx_expected_len = 0U;
}

static void send_frame(const uint8_t *frame, size_t len)
{
    if (g_port_ops.set_tx_enable != NULL) {
        g_port_ops.set_tx_enable(true);
    } else {
        X42S_PortSetTxEnable(true);
    }

    if (g_port_ops.send != NULL) {
        g_port_ops.send(frame, len);
    } else {
        X42S_PortSend(frame, len);
    }

    if (g_port_ops.set_tx_enable != NULL) {
        g_port_ops.set_tx_enable(false);
    } else {
        X42S_PortSetTxEnable(false);
    }
}

static uint8_t direction_from_signed(int32_t value)
{
    return (value < 0) ? X42S_DIR_CCW : X42S_DIR_CW;
}

void X42S_SetPortOps(const X42S_PortOps *ops)
{
    if (ops == NULL) {
        g_port_ops.send = NULL;
        g_port_ops.set_tx_enable = NULL;
        return;
    }

    g_port_ops = *ops;
}

void X42S_Enable(uint8_t id)
{
    uint8_t frame[] = {id, 0xF3U, 0xABU, 0x01U, 0x00U, X42S_CHECK_FIXED};
    send_frame(frame, sizeof(frame));
}

void X42S_Disable(uint8_t id)
{
    uint8_t frame[] = {id, 0xF3U, 0xABU, 0x00U, 0x00U, X42S_CHECK_FIXED};
    send_frame(frame, sizeof(frame));
}

void X42S_SetPosition(uint8_t id, int32_t position)
{
    X42S_SetPositionEx(id, position, X42S_DEFAULT_ACC_RPM_S,
                       X42S_DEFAULT_DEC_RPM_S, X42S_DEFAULT_SPEED_0P1_RPM,
                       0U, false);
}

void X42S_NudgeRelative(uint8_t id, int32_t delta_0p1deg)
{
    X42S_SetPositionEx(id, delta_0p1deg, X42S_DEFAULT_ACC_RPM_S,
                       X42S_DEFAULT_DEC_RPM_S, X42S_DEFAULT_SPEED_0P1_RPM,
                       1U, false);
}

void X42S_SetSpeed(uint8_t id, int32_t speed)
{
    X42S_SetSpeedEx(id, speed, X42S_DEFAULT_ACC_RPM_S, false);
}

int32_t X42S_ReadPosition(uint8_t id)
{
    uint8_t frame[] = {id, 0x36U, X42S_CHECK_FIXED};
    send_frame(frame, sizeof(frame));
    return X42S_GetLastPosition(id);
}

void X42S_Stop(uint8_t id)
{
    uint8_t frame[] = {id, 0xFEU, 0x98U, 0x00U, X42S_CHECK_FIXED};
    send_frame(frame, sizeof(frame));
}

void X42S_SetPositionEx(uint8_t id, int32_t position_0p1deg,
                        uint16_t acc_rpm_s, uint16_t dec_rpm_s,
                        uint16_t speed_0p1rpm, uint8_t raf, bool sync)
{
    uint8_t frame[13] = {0};
    uint32_t pulses = angle_0p1deg_to_pulses(position_0p1deg);

    (void)dec_rpm_s;
    frame[0] = id;
    frame[1] = 0xFDU;
    frame[2] = direction_from_signed(position_0p1deg);
    put_u16(&frame[3], speed_0p1rpm_to_rpm(speed_0p1rpm));
    frame[5] = acc_rpm_s_to_emm(acc_rpm_s);
    put_u32(&frame[6], pulses);
    frame[10] = (raf != 0U) ? 1U : 0U;
    frame[11] = sync ? 1U : 0U;
    frame[12] = X42S_CHECK_FIXED;

    send_frame(frame, sizeof(frame));
}

void X42S_SetSpeedEx(uint8_t id, int32_t speed_0p1rpm,
                     uint16_t acc_rpm_s, bool sync)
{
    uint8_t frame[8] = {0};

    frame[0] = id;
    frame[1] = 0xF6U;
    frame[2] = direction_from_signed(speed_0p1rpm);
    put_u16(&frame[3], speed_0p1rpm_to_rpm((uint16_t)abs_i32(speed_0p1rpm)));
    frame[5] = acc_rpm_s_to_emm(acc_rpm_s);
    frame[6] = sync ? 1U : 0U;
    frame[7] = X42S_CHECK_FIXED;

    send_frame(frame, sizeof(frame));
}

void X42S_RequestSpeed(uint8_t id)
{
    uint8_t frame[] = {id, 0x35U, X42S_CHECK_FIXED};
    send_frame(frame, sizeof(frame));
}

int32_t X42S_GetLastPosition(uint8_t id)
{
    if (id > X42S_MAX_ID) {
        return 0;
    }

    return g_last_position[id];
}

int32_t X42S_GetLastSpeed(uint8_t id)
{
    if (id > X42S_MAX_ID) {
        return 0;
    }

    return g_last_speed[id];
}

uint8_t X42S_GetLastReplyId(void)
{
    return g_last_reply_id;
}

uint8_t X42S_GetLastReplyCommand(void)
{
    return g_last_reply_command;
}

uint16_t X42S_GetPositionUpdateCount(uint8_t id)
{
    if (id > X42S_MAX_ID) {
        return 0U;
    }

    return g_position_update_count[id];
}

void X42S_OnRxByte(uint8_t byte)
{
    uint8_t id;
    uint8_t cmd;

    if (g_rx_len == 0U) {
        if (byte == 0U || byte > X42S_MAX_ID) {
            return;
        }
        g_rx_buf[g_rx_len++] = byte;
        return;
    }
    if (g_rx_len >= sizeof(g_rx_buf)) {
        reset_rx();
        return;
    }
    g_rx_buf[g_rx_len++] = byte;

    if (g_rx_len == 2U) {
        g_rx_expected_len = reply_length_for_command(g_rx_buf[1]);
        if (g_rx_expected_len == 0U) {
            reset_rx();
        }
        return;
    }
    if (g_rx_len < g_rx_expected_len) {
        return;
    }
    if (g_rx_len != g_rx_expected_len || byte != X42S_CHECK_FIXED) {
        reset_rx();
        return;
    }

    id = g_rx_buf[0];
    cmd = g_rx_buf[1];
    g_last_reply_id = id;
    g_last_reply_command = cmd;

    if (cmd == 0x35U) {
        uint8_t sign = g_rx_buf[2];
        int32_t speed = (int32_t)get_u16(&g_rx_buf[3]);
        g_last_speed[id] = sign ? -(speed * 10) : (speed * 10);
    } else if (cmd == 0x36U) {
        uint8_t sign = g_rx_buf[2];
        g_last_position[id] = encoder_counts_to_angle_0p1deg(
            get_u32(&g_rx_buf[3]), sign);
        g_position_update_count[id]++;
    }

    reset_rx();
}

X42S_WEAK void X42S_PortSend(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
}

X42S_WEAK void X42S_PortSetTxEnable(bool enable)
{
    (void)enable;
}
