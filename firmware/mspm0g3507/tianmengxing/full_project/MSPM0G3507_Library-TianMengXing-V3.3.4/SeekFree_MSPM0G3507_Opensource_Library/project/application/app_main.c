#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "app_main.h"
#include "car/car_chassis.h"
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

    MaixCAM_ProtocolInit(&g_vision_parser);
    X42S_SetPortOps(&motor_port);
    Gimbal_Init(&g_gimbal);
    g_vision_timeout_reported = true;
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
            App_DebugLog("VISION,FRAME\r\n");
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
