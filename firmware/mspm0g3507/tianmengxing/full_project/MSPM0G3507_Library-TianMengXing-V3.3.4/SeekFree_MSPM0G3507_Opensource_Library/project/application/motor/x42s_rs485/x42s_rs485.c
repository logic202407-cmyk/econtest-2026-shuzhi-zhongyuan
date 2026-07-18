#include "x42s_rs485.h"

#if X42S_FIRMWARE_X_FREE != 1U
#error "The current x42s_rs485 driver supports only X firmware free protocol."
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
static uint8_t g_rx_buf[16];
static uint8_t g_rx_len;
static X42S_PortOps g_port_ops;

static uint32_t abs_i32(int32_t value)
{
    if (value < 0) {
        return (uint32_t)(-(value + 1)) + 1U;
    }

    return (uint32_t)value;
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
                       2U, false);
}

void X42S_SetSpeed(uint8_t id, int32_t speed)
{
    X42S_SetSpeedEx(id, speed, X42S_DEFAULT_ACC_RPM_S, false);
}

int32_t X42S_ReadPosition(uint8_t id)
{
    uint8_t frame[] = {id, 0x0FU, X42S_CHECK_FIXED};
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
    uint8_t frame[16] = {0};

    frame[0] = id;
    frame[1] = 0xFDU;
    frame[2] = direction_from_signed(position_0p1deg);
    put_u16(&frame[3], acc_rpm_s);
    put_u16(&frame[5], dec_rpm_s);
    put_u16(&frame[7], speed_0p1rpm);
    put_u32(&frame[9], abs_i32(position_0p1deg));
    frame[13] = raf;
    frame[14] = sync ? 1U : 0U;
    frame[15] = X42S_CHECK_FIXED;

    send_frame(frame, sizeof(frame));
}

void X42S_SetSpeedEx(uint8_t id, int32_t speed_0p1rpm,
                     uint16_t acc_rpm_s, bool sync)
{
    uint8_t frame[9] = {0};

    frame[0] = id;
    frame[1] = 0xF6U;
    frame[2] = direction_from_signed(speed_0p1rpm);
    put_u16(&frame[3], acc_rpm_s);
    put_u16(&frame[5], (uint16_t)abs_i32(speed_0p1rpm));
    frame[7] = sync ? 1U : 0U;
    frame[8] = X42S_CHECK_FIXED;

    send_frame(frame, sizeof(frame));
}

void X42S_RequestSpeed(uint8_t id)
{
    uint8_t frame[] = {id, 0x0EU, X42S_CHECK_FIXED};
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

void X42S_OnRxByte(uint8_t byte)
{
    if (g_rx_len < sizeof(g_rx_buf)) {
        g_rx_buf[g_rx_len++] = byte;
    } else {
        g_rx_len = 0;
    }

    if (byte != X42S_CHECK_FIXED) {
        return;
    }

    if (g_rx_len >= 6U) {
        uint8_t id = g_rx_buf[0];
        uint8_t cmd = g_rx_buf[1];
        uint8_t sign = g_rx_buf[2];

        if (id <= X42S_MAX_ID && cmd == 0x0EU && g_rx_len >= 6U) {
            int32_t speed = (int32_t)get_u16(&g_rx_buf[3]);
            g_last_speed[id] = sign ? -speed : speed;
        } else if (id <= X42S_MAX_ID && cmd == 0x0FU && g_rx_len >= 8U) {
            int32_t position = (int32_t)get_u32(&g_rx_buf[3]);
            g_last_position[id] = sign ? -position : position;
        }
    }

    g_rx_len = 0;
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
