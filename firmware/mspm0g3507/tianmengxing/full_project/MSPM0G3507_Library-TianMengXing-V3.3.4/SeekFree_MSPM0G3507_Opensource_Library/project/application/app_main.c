#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "app_main.h"
#include "car/car_chassis.h"
#include "car/car_gyro.h"
#include "car/car_demo.h"
#include "car/car_gray.h"
#include "config/app_config.h"
#include "display/tjc_screen/tjc_screen.h"
#include "gimbal/gimbal_control.h"
#include "motor/x42s_rs485/x42s_rs485.h"
#include "vision/maixcam_protocol.h"

#if defined(__CC_ARM) && !defined(__clang__)
#define APP_WEAK __weak
#else
#define APP_WEAK __attribute__((weak))
#endif

static MaixCAM_Parser g_vision_parser;
static Gimbal_Control g_gimbal;
static bool g_vision_timeout_reported;
#if CAR_GYRO_ENABLED
static uint8_t g_car_gyro_last_reported_status = 0xFFU;
#endif
#if APP_GIMBAL_DEBUG_ENABLED
static uint32_t g_last_gimbal_debug_ms;
#endif

/* Platform hooks are implemented by the TianMengXing project integration. */
APP_WEAK void App_SystemInit(void) {}
APP_WEAK void App_PlatformInit(void) {}
APP_WEAK void App_DebugUartInit(void) {}
APP_WEAK void App_DebugLog(const char *message)
{
    (void)message;
}
APP_WEAK void App_VisionUartInit(void) {}
APP_WEAK void App_MotorUartInit(void) {}
APP_WEAK uint32_t App_GetMillis(void) { return 0U; }
APP_WEAK bool App_VisionReadByte(uint8_t *byte)
{
    (void)byte;
    return false;
}

APP_WEAK bool App_MotorReadByte(uint8_t *byte)
{
    (void)byte;
    return false;
}

/* This hook must return only after the motor UART has shifted the last byte. */
APP_WEAK void App_MotorSend(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
}

APP_WEAK void App_Rs485SetTxEnable(bool enable)
{
    (void)enable;
}

APP_WEAK void App_BoardService(uint32_t now_ms)
{
    (void)now_ms;
}

APP_WEAK void App_Idle(void) {}

static void motor_send(const uint8_t *data, size_t len)
{
    App_MotorSend(data, len);
}

static void motor_set_tx_enable(bool enable)
{
    App_Rs485SetTxEnable(enable);
}

static void log_vision_frame(const MaixCAM_Parser *parser)
{
    char debug_message[96];

    if (parser->target.valid) {
        long yaw = (long)parser->target.yaw_0p01deg;
        long pitch = (long)parser->target.pitch_0p01deg;
        unsigned long confidence = (unsigned long)parser->target.confidence_0p01pct;
        long yaw_abs = (yaw < 0L) ? -yaw : yaw;
        long pitch_abs = (pitch < 0L) ? -pitch : pitch;

        (void)snprintf(debug_message, sizeof(debug_message),
                       "VISION,TARGET,YAW,%ld.%02ld,PITCH,%ld.%02ld,CONF,%lu.%02lu\r\n",
                       yaw / 100L,
                       yaw_abs % 100L,
                       pitch / 100L,
                       pitch_abs % 100L,
                       confidence / 100UL,
                       confidence % 100UL);
    } else {
        (void)snprintf(debug_message, sizeof(debug_message),
                       "VISION,LOST\r\n");
    }

    App_DebugLog(debug_message);
}

#if APP_GIMBAL_DEBUG_ENABLED
static void log_gimbal_state(uint32_t now_ms)
{
    char debug_message[128];
    MaixCAM_Target target = MaixCAM_GetTarget(&g_vision_parser);

    if ((uint32_t)(now_ms - g_last_gimbal_debug_ms) <
        APP_GIMBAL_DEBUG_PERIOD_MS) {
        return;
    }

    g_last_gimbal_debug_ms = now_ms;
    (void)snprintf(debug_message, sizeof(debug_message),
                   "GIMBAL,YAW,%ld,FILT,%ld,SPD,%ld,POS,%ld,STOP,%u\r\n",
                   (long)target.yaw_0p01deg,
                   (long)g_gimbal.filtered_yaw_0p01deg,
                   (long)g_gimbal.yaw_speed_0p1rpm,
                   (long)g_gimbal.yaw_position_0p1deg,
                   g_gimbal.stopped ? 1U : 0U);
    App_DebugLog(debug_message);
}
#endif

void App_Init(void)
{
    static const X42S_PortOps motor_port = {
        motor_send,
        motor_set_tx_enable
    };

    App_SystemInit();
    App_PlatformInit();
    App_DebugUartInit();
    App_VisionUartInit();
    App_MotorUartInit();

#if TJC_SCREEN_ENABLED
    TJC_ScreenInit();
#endif
#if CAR_MOTION_ENABLED
    CarGray_Init();
    CarChassis_Init();
#endif
#if CAR_GYRO_ENABLED
    CarGyro_Init(App_GetMillis());
    App_DebugLog("CAR,GYRO,INIT,A31-SCL,A28-SDA\r\n");
#endif

    MaixCAM_ProtocolInit(&g_vision_parser);
    X42S_SetPortOps(&motor_port);
    Gimbal_Init(&g_gimbal);
    g_vision_timeout_reported = true;
#if APP_GIMBAL_DEBUG_ENABLED
    g_last_gimbal_debug_ms = 0U;
#endif
    App_DebugLog("APP,INIT\r\n");
}

void App_MainLoopOnce(void)
{
    uint8_t byte;
    uint32_t now_ms = App_GetMillis();
    uint8_t count;

    for (count = 0U; count < APP_UART_POLL_BUDGET; ++count) {
        if (!App_VisionReadByte(&byte)) {
            break;
        }

#if APP_VISION_RX_DEBUG_ENABLED
        {
            char debug_message[24];
            (void)snprintf(debug_message, sizeof(debug_message),
                           "VISION,BYTE,%02X\r\n",
                           (unsigned int)byte);
            App_DebugLog(debug_message);
        }
#endif
        if (MaixCAM_ProtocolInputByte(&g_vision_parser, byte, now_ms)) {
            g_vision_timeout_reported = false;
            log_vision_frame(&g_vision_parser);
        }
    }

    for (count = 0U; count < APP_UART_POLL_BUDGET; ++count) {
        if (!App_MotorReadByte(&byte)) {
            break;
        }

        X42S_OnRxByte(byte);
#if APP_MOTOR_RX_DEBUG_ENABLED
        if (byte == X42S_CHECK_FIXED) {
            uint8_t reply_id = X42S_GetLastReplyId();
            uint8_t reply_command = X42S_GetLastReplyCommand();
            char debug_message[64];
            long position_0p1deg;
            long position_fraction;

            if (reply_command == 0x36U) {
                position_0p1deg = (long)X42S_GetLastPosition(reply_id);
                position_fraction = (position_0p1deg < 0L) ? -position_0p1deg : position_0p1deg;
                (void)snprintf(debug_message, sizeof(debug_message),
                               "X42S,POS,ID%u,%ld.%ldDEG\r\n",
                               (unsigned int)reply_id,
                               position_0p1deg / 10L,
                               position_fraction % 10L);
            } else if (reply_command == 0x35U) {
                (void)snprintf(debug_message, sizeof(debug_message),
                               "X42S,SPEED,ID%u,%ld0P1RPM\r\n",
                               (unsigned int)reply_id,
                               (long)X42S_GetLastSpeed(reply_id));
            } else {
                (void)snprintf(debug_message, sizeof(debug_message),
                               "X42S,ACK,ID%u,CMD,%02X\r\n",
                               (unsigned int)reply_id,
                               (unsigned int)reply_command);
            }
            App_DebugLog(debug_message);
        }
#endif
    }

    Gimbal_Update(&g_gimbal, &g_vision_parser, now_ms);
#if CAR_GYRO_ENABLED
    CarGyro_Service(now_ms);
    if (g_car_gyro_last_reported_status != (uint8_t)CarGyro_GetStatus()) {
        char gyro_log[40];

        g_car_gyro_last_reported_status = (uint8_t)CarGyro_GetStatus();
        (void)snprintf(gyro_log, sizeof(gyro_log),
                       "CAR,GYRO,STATUS,%u\r\n",
                       (unsigned int)g_car_gyro_last_reported_status);
        App_DebugLog(gyro_log);
    }
#endif
#if APP_GIMBAL_DEBUG_ENABLED
    log_gimbal_state(now_ms);
#endif
    if (!g_vision_timeout_reported && MaixCAM_IsTimeout(&g_vision_parser, now_ms)) {
        g_vision_timeout_reported = true;
        App_DebugLog("VISION,TIMEOUT\r\n");
    }
    App_BoardService(now_ms);
    App_Idle();
}

void App_Run(void)
{
#if APP_GIMBAL_RS485_TEST_BOOT_ENABLED
    App_Init();

    while (true) {
        App_MainLoopOnce();
    }
#elif CAR_DEMO_AUTORUN_ENABLED
    CarDemo_Run();
#else
    App_Init();

    while (true) {
        App_MainLoopOnce();
    }
#endif
}
