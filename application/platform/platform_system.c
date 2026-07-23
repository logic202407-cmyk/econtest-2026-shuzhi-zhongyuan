#include "platform_system.h"

#include <string.h>

#include "zf_common_headfile.h"

#include "../config/app_config.h"
#include "platform_gpio.h"
#include "platform_uart.h"
#include "../car/car_chassis.h"
#include "../motor/x42s_rs485/x42s_rs485.h"

static volatile uint32_t g_platform_tick_ms;

static void platform_tick_handler(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;
    ++g_platform_tick_ms;
    CarChassis_ControlTick1ms();
}

void platform_system_init(void)
{
    clock_init(SYSTEM_CLOCK_80M);
}

void platform_time_init(void)
{
    g_platform_tick_ms = 0U;
    pit_ms_init(PIT_TIM_A0, 1U, platform_tick_handler, NULL);
}

uint32_t platform_time_ms(void)
{
    return g_platform_tick_ms;
}

void App_SystemInit(void)
{
    platform_system_init();
}

void App_PlatformInit(void)
{
    tmx_board_init();
    platform_gpio_init();
    platform_time_init();
}

void App_DebugUartInit(void)
{
    debug_init();
}

void App_DebugLog(const char *message)
{
#if APP_DEBUG_LOG_ENABLED
    if (message != NULL) {
        (void)debug_send_buffer((const uint8 *)message, (uint32)strlen(message));
    }
#else
    (void)message;
#endif
}

uint32_t App_GetMillis(void)
{
    return platform_time_ms();
}

#if APP_X42S_KEY_TEST_ENABLED
static void x42s_key_test_step(void)
{
    static uint8_t step;

    switch (step) {
    case 0U:
        App_DebugLog("X42S,ENABLE,READ\r\n");
        X42S_Enable(X42S_PITCH_MOTOR_ID);
        X42S_Enable(X42S_YAW_MOTOR_ID);
        (void)X42S_ReadPosition(X42S_PITCH_MOTOR_ID);
        (void)X42S_ReadPosition(X42S_YAW_MOTOR_ID);
        X42S_RequestSpeed(X42S_PITCH_MOTOR_ID);
        X42S_RequestSpeed(X42S_YAW_MOTOR_ID);
        break;

    case 1U:
        App_DebugLog("X42S,NUDGE,+20DEG,REL\r\n");
        X42S_NudgeRelative(X42S_PITCH_MOTOR_ID, 200);
        X42S_NudgeRelative(X42S_YAW_MOTOR_ID, 200);
        break;

    case 2U:
        App_DebugLog("X42S,NUDGE,-20DEG,REL\r\n");
        X42S_NudgeRelative(X42S_PITCH_MOTOR_ID, -200);
        X42S_NudgeRelative(X42S_YAW_MOTOR_ID, -200);
        break;

    default:
        App_DebugLog("X42S,STOP,BOTH\r\n");
        X42S_Stop(X42S_PITCH_MOTOR_ID);
        X42S_Stop(X42S_YAW_MOTOR_ID);
        break;
    }

    step = (uint8_t)((step + 1U) & 0x03U);
}
#endif

void App_BoardService(uint32_t now_ms)
{
#if APP_BOARD_SELF_TEST_ENABLED
    static uint32_t last_led_toggle_ms;
    static uint32_t last_vision_tx_ms;
    static bool key_was_pressed;

    if ((now_ms - last_led_toggle_ms) >= APP_BOARD_LED_HEARTBEAT_MS) {
        last_led_toggle_ms = now_ms;
        platform_led_toggle();
    }

    bool key_is_pressed = platform_key_read();
    if (key_is_pressed != key_was_pressed) {
        key_was_pressed = key_is_pressed;
        App_DebugLog(key_is_pressed ? "KEY,PRESS,B21\r\n" : "KEY,RELEASE,B21\r\n");
#if APP_X42S_KEY_TEST_ENABLED
        if (key_is_pressed) {
            x42s_key_test_step();
        }
#endif
    }

#if APP_VISION_TX_SELF_TEST_ENABLED
    if ((now_ms - last_vision_tx_ms) >= APP_VISION_TX_SELF_TEST_MS) {
        static const uint8_t message[] = "UART1,TX,A8\r\n";
        last_vision_tx_ms = now_ms;
        platform_uart_send(PLATFORM_UART_VISION, message, sizeof(message) - 1U);
    }
#endif
#else
    (void)now_ms;
#endif
}
