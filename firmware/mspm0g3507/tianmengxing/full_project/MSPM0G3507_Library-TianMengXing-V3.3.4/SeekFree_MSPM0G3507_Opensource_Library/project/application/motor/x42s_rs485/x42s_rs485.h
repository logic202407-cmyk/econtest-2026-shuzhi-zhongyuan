#ifndef X42S_RS485_H
#define X42S_RS485_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../../config/app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define X42S_CHECK_FIXED              0x6BU
#define X42S_DEFAULT_ACC_RPM_S        X42S_DEFAULT_ACC_RPM_S_CONFIG
#define X42S_DEFAULT_DEC_RPM_S        X42S_DEFAULT_DEC_RPM_S_CONFIG
#define X42S_DEFAULT_SPEED_0P1_RPM    X42S_DEFAULT_SPEED_0P1_RPM_CFG
#define X42S_UART_INDEX               APP_MOTOR_UART_INDEX
#define X42S_UART_TX_PIN              APP_MOTOR_UART_TX_PIN
#define X42S_UART_RX_PIN              APP_MOTOR_UART_RX_PIN
#define X42S_RS485_DE_PIN             RS485_DE_PIN

typedef enum
{
    X42S_DIR_CW = 0,
    X42S_DIR_CCW = 1
} X42S_Direction;

typedef struct
{
    void (*send)(const uint8_t *data, size_t len);
    void (*set_tx_enable)(bool enable);
} X42S_PortOps;

void X42S_SetPortOps(const X42S_PortOps *ops);
void X42S_Enable(uint8_t id);
void X42S_Disable(uint8_t id);
void X42S_SetPosition(uint8_t id, int32_t position);
void X42S_SetSpeed(uint8_t id, int32_t speed);
int32_t X42S_ReadPosition(uint8_t id);

void X42S_Stop(uint8_t id);
void X42S_SetPositionEx(uint8_t id, int32_t position_0p1deg,
                        uint16_t acc_rpm_s, uint16_t dec_rpm_s,
                        uint16_t speed_0p1rpm, uint8_t raf, bool sync);
void X42S_SetSpeedEx(uint8_t id, int32_t speed_0p1rpm,
                     uint16_t acc_rpm_s, bool sync);
void X42S_RequestSpeed(uint8_t id);
int32_t X42S_GetLastPosition(uint8_t id);
int32_t X42S_GetLastSpeed(uint8_t id);
void X42S_OnRxByte(uint8_t byte);

void X42S_PortSend(const uint8_t *data, size_t len);
void X42S_PortSetTxEnable(bool enable);

#ifdef __cplusplus
}
#endif

#endif
