#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config/app_config.h"
#include "gimbal/gimbal_control.h"
#include "motor/x42s_rs485/x42s_rs485.h"
#include "vision/maixcam_protocol.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define APP_WEAK __weak
#else
#define APP_WEAK __attribute__((weak))
#endif

static MaixCAM_Parser g_vision_parser;
static Gimbal_Control g_gimbal;

/* Platform hooks are implemented by the TianMengXing project integration. */
APP_WEAK void App_SystemInit(void) {}
APP_WEAK void App_PlatformInit(void) {}
APP_WEAK void App_DebugUartInit(void) {}
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

/* This hook must return only after UART2 has shifted the last byte. */
APP_WEAK void App_MotorSend(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
}

APP_WEAK void App_Rs485SetTxEnable(bool enable)
{
    (void)enable;
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

    MaixCAM_ProtocolInit(&g_vision_parser);
    X42S_SetPortOps(&motor_port);
    Gimbal_Init(&g_gimbal);
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

        (void)MaixCAM_ProtocolInputByte(&g_vision_parser, byte, now_ms);
    }

    for (count = 0U; count < APP_UART_POLL_BUDGET; ++count) {
        if (!App_MotorReadByte(&byte)) {
            break;
        }

        X42S_OnRxByte(byte);
    }

    Gimbal_Update(&g_gimbal, &g_vision_parser, now_ms);
    App_Idle();
}

void App_Run(void)
{
    App_Init();

    while (true) {
        App_MainLoopOnce();
    }
}
