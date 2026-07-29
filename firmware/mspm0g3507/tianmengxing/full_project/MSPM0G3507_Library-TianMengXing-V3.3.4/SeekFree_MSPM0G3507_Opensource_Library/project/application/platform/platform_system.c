#include "platform_system.h"

#include <stdio.h>
#include <string.h>

#include "zf_common_headfile.h"

#include "../config/app_config.h"
#include "platform_gpio.h"
#include "platform_uart.h"
#include "../car/car_chassis.h"
#include "../motor/x42s_rs485/x42s_rs485.h"
#if APP_OLED_TIMER_SCREEN_ENABLED
#include "../display/oled_i2c/oled_i2c.h"
#endif

static volatile uint32_t g_platform_tick_ms;

#if APP_SERVO_KEY_TEST_ENABLED
static void servo_test_init(void);
static void servo_key_test_step(void);
#endif
#if APP_IPS200PRO_SCREEN_ENABLED
static void ips200pro_test_init(void);
static void ips200pro_time_service(uint32_t now_ms);
#endif
#if APP_OLED_TIMER_SCREEN_ENABLED
static void oled_timer_screen_init(void);
static void oled_timer_screen_service(uint32_t now_ms);
#endif
#if APP_CAR_KEY_TEST_ENABLED
static void car_key_test_step(void);
#endif

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
#if APP_SERVO_KEY_TEST_ENABLED
    servo_test_init();
#endif
#if APP_IPS200PRO_SCREEN_ENABLED
    ips200pro_test_init();
#endif
#if APP_OLED_TIMER_SCREEN_ENABLED
    oled_timer_screen_init();
#endif
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
typedef enum
{
    X42S_TEST_PENDING_NONE = 0,
    X42S_TEST_PENDING_ENABLE_YAW,
    X42S_TEST_PENDING_READ_PITCH,
    X42S_TEST_PENDING_READ_YAW,
    X42S_TEST_PENDING_NUDGE_YAW,
    X42S_TEST_PENDING_STOP_YAW
} X42S_TestPending;

static X42S_TestPending g_x42s_test_pending;
static int32_t g_x42s_test_nudge_0p1deg;
static uint32_t g_x42s_test_due_ms;

static void x42s_key_test_schedule(X42S_TestPending pending, uint32_t now_ms)
{
    g_x42s_test_pending = pending;
    g_x42s_test_due_ms = now_ms + X42S_TEST_INTERFRAME_MS;
}

static void x42s_key_test_step(uint32_t now_ms)
{
    static uint8_t step;

    switch (step) {
    case 0U:
        App_DebugLog("X42S,ENABLE,READ,SEQ\r\n");
        X42S_Enable(X42S_PITCH_MOTOR_ID);
        x42s_key_test_schedule(X42S_TEST_PENDING_ENABLE_YAW, now_ms);
        break;

    case 1U:
        App_DebugLog("X42S,NUDGE,+20DEG,REL\r\n");
        X42S_NudgeRelative(X42S_PITCH_MOTOR_ID, 200);
        g_x42s_test_nudge_0p1deg = 200;
        x42s_key_test_schedule(X42S_TEST_PENDING_NUDGE_YAW, now_ms);
        break;

    case 2U:
        App_DebugLog("X42S,NUDGE,-20DEG,REL\r\n");
        X42S_NudgeRelative(X42S_PITCH_MOTOR_ID, -200);
        g_x42s_test_nudge_0p1deg = -200;
        x42s_key_test_schedule(X42S_TEST_PENDING_NUDGE_YAW, now_ms);
        break;

    default:
        App_DebugLog("X42S,STOP,BOTH\r\n");
        X42S_Stop(X42S_PITCH_MOTOR_ID);
        x42s_key_test_schedule(X42S_TEST_PENDING_STOP_YAW, now_ms);
        break;
    }

    step = (uint8_t)((step + 1U) & 0x03U);
}

static void x42s_key_test_service(uint32_t now_ms)
{
    if (g_x42s_test_pending == X42S_TEST_PENDING_NONE ||
        now_ms < g_x42s_test_due_ms) {
        return;
    }

    switch (g_x42s_test_pending) {
    case X42S_TEST_PENDING_ENABLE_YAW:
        X42S_Enable(X42S_YAW_MOTOR_ID);
        x42s_key_test_schedule(X42S_TEST_PENDING_READ_PITCH, now_ms);
        break;

    case X42S_TEST_PENDING_READ_PITCH:
        (void)X42S_ReadPosition(X42S_PITCH_MOTOR_ID);
        x42s_key_test_schedule(X42S_TEST_PENDING_READ_YAW, now_ms);
        break;

    case X42S_TEST_PENDING_READ_YAW:
        (void)X42S_ReadPosition(X42S_YAW_MOTOR_ID);
        g_x42s_test_pending = X42S_TEST_PENDING_NONE;
        break;

    case X42S_TEST_PENDING_NUDGE_YAW:
        X42S_NudgeRelative(X42S_YAW_MOTOR_ID, g_x42s_test_nudge_0p1deg);
        g_x42s_test_pending = X42S_TEST_PENDING_NONE;
        break;

    case X42S_TEST_PENDING_STOP_YAW:
        X42S_Stop(X42S_YAW_MOTOR_ID);
        g_x42s_test_pending = X42S_TEST_PENDING_NONE;
        break;

    default:
        g_x42s_test_pending = X42S_TEST_PENDING_NONE;
        break;
    }
}
#endif

#if APP_SERVO_KEY_TEST_ENABLED
static uint16_t servo_angle_to_pulse_us(uint8_t angle_deg)
{
    uint32_t pulse_range = APP_SERVO_TEST_MAX_US - APP_SERVO_TEST_MIN_US;
    uint32_t pulse_us = APP_SERVO_TEST_MIN_US +
                        ((uint32_t)angle_deg * pulse_range + 90U) / 180U;

    if (pulse_us < APP_SERVO_TEST_MIN_US) {
        pulse_us = APP_SERVO_TEST_MIN_US;
    } else if (pulse_us > APP_SERVO_TEST_MAX_US) {
        pulse_us = APP_SERVO_TEST_MAX_US;
    }

    return (uint16_t)pulse_us;
}

static uint32_t servo_pulse_us_to_duty(uint16_t pulse_us)
{
    uint32_t period_us = 1000000UL / APP_SERVO_TEST_PWM_FREQ_HZ;

    return ((uint32_t)pulse_us * PWM_DUTY_MAX + (period_us / 2U)) /
           period_us;
}

static void servo_test_set_angle(uint8_t angle_deg)
{
    uint16_t pulse_us = servo_angle_to_pulse_us(angle_deg);
    uint32_t duty = servo_pulse_us_to_duty(pulse_us);
    char message[48];

    pwm_set_duty(APP_SERVO_TEST_PWM_PIN, duty);
    (void)snprintf(message, sizeof(message),
                   "SERVO,B2,%uDEG,%uUS\r\n",
                   (unsigned int)angle_deg,
                   (unsigned int)pulse_us);
    App_DebugLog(message);
}

static void servo_test_init(void)
{
    uint16_t pulse_us = servo_angle_to_pulse_us(APP_SERVO_TEST_START_DEG);

    pwm_init(APP_SERVO_TEST_PWM_PIN, APP_SERVO_TEST_PWM_FREQ_HZ,
             servo_pulse_us_to_duty(pulse_us));
    App_DebugLog("SERVO,INIT,B2,50HZ\r\n");
    servo_test_set_angle(APP_SERVO_TEST_START_DEG);
}

static void servo_key_test_step(void)
{
    static const uint8_t angles[] = {
        APP_SERVO_TEST_START_DEG,
        APP_SERVO_TEST_START_DEG - APP_SERVO_TEST_STEP_DEG,
        APP_SERVO_TEST_START_DEG + APP_SERVO_TEST_STEP_DEG,
        APP_SERVO_TEST_START_DEG
    };
    static uint8_t index = 1U;

    servo_test_set_angle(angles[index]);
    index = (uint8_t)((index + 1U) % (sizeof(angles) / sizeof(angles[0])));
}

static void servo_auto_sweep_service(uint32_t now_ms)
{
#if APP_SERVO_TEST_AUTO_SWEEP_ENABLED
    static const uint8_t angles[] = {
        APP_SERVO_TEST_START_DEG - APP_SERVO_TEST_STEP_DEG,
        APP_SERVO_TEST_START_DEG + APP_SERVO_TEST_STEP_DEG,
        APP_SERVO_TEST_START_DEG
    };
    static uint32_t last_sweep_ms;
    static uint8_t index;

    if ((uint32_t)(now_ms - last_sweep_ms) < APP_SERVO_TEST_AUTO_SWEEP_MS) {
        return;
    }

    last_sweep_ms = now_ms;
    servo_test_set_angle(angles[index]);
    index = (uint8_t)((index + 1U) % (sizeof(angles) / sizeof(angles[0])));
#else
    (void)now_ms;
#endif
}
#endif

#if APP_IPS200PRO_SCREEN_ENABLED
static uint16_t g_ips200pro_time_label_id;
static uint32_t g_ips200pro_last_time_ms;

static void ips200pro_show_uptime(uint32_t now_ms)
{
    uint32_t total_seconds = now_ms / 1000U;
    uint32_t hours = total_seconds / 3600U;
    uint32_t minutes = (total_seconds / 60U) % 60U;
    uint32_t seconds = total_seconds % 60U;
    char text[24];

    if (g_ips200pro_time_label_id == 0U) {
        return;
    }

    (void)snprintf(text, sizeof(text), "TIME %02lu:%02lu:%02lu",
                   (unsigned long)(hours % 100U),
                   (unsigned long)minutes,
                   (unsigned long)seconds);
    (void)ips200pro_label_show_string(g_ips200pro_time_label_id, text);
}

static void ips200pro_test_init(void)
{
    uint16_t page_id;
    uint16_t label_id;

    g_ips200pro_time_label_id = 0U;
    g_ips200pro_last_time_ms = 0U;

    page_id = ips200pro_init((char *)"TIME TEST", IPS200PRO_TITLE_TOP, 24U);
    if (page_id == 0U) {
        App_DebugLog("IPS200PRO,INIT,FAIL\r\n");
        return;
    }

    label_id = ips200pro_label_create(8, 40, 220, 28);
    if (label_id != 0U) {
        (void)ips200pro_label_show_string(label_id, "IPS200PRO OK");
    }

    label_id = ips200pro_label_create(8, 76, 220, 28);
    if (label_id != 0U) {
        (void)ips200pro_label_show_string(label_id, "UPTIME CLOCK");
    }

    g_ips200pro_time_label_id = ips200pro_label_create(8, 112, 220, 32);
    ips200pro_show_uptime(App_GetMillis());

    App_DebugLog("IPS200PRO,TIME,INIT,OK\r\n");
}

static void ips200pro_time_service(uint32_t now_ms)
{
    if ((uint32_t)(now_ms - g_ips200pro_last_time_ms) < 1000U) {
        return;
    }

    g_ips200pro_last_time_ms = now_ms;
    ips200pro_show_uptime(now_ms);
}
#endif

#if APP_OLED_TIMER_SCREEN_ENABLED
static uint32_t g_oled_timer_start_ms;
static uint32_t g_oled_timer_last_show_ms;

static void oled_timer_screen_show(uint32_t elapsed_ms)
{
    uint32_t total_seconds = elapsed_ms / 1000U;
    uint32_t minutes = (total_seconds / 60U) % 100U;
    uint32_t seconds = total_seconds % 60U;
    char text[8];

    (void)snprintf(text, sizeof(text), "%02lu:%02lu",
                   (unsigned long)minutes,
                   (unsigned long)seconds);

    OledI2c_Clear();
    OledI2c_ShowString(46U, 0U, "TIMER");
    OledI2c_ShowLargeString(19U, 3U, text, 3U);
}

static void oled_timer_screen_init(void)
{
    OledI2c_Init();
    g_oled_timer_start_ms = App_GetMillis();
    g_oled_timer_last_show_ms = 0xFFFFFFFFUL;
    oled_timer_screen_show(0U);
    App_DebugLog("OLED,TIMER,INIT,B4-SCK,B5-SDA\r\n");
}

static void oled_timer_screen_service(uint32_t now_ms)
{
    uint32_t elapsed_ms = now_ms - g_oled_timer_start_ms;
    uint32_t elapsed_seconds = elapsed_ms / 1000U;

    if (elapsed_seconds == g_oled_timer_last_show_ms) {
        return;
    }

    g_oled_timer_last_show_ms = elapsed_seconds;
    oled_timer_screen_show(elapsed_ms);
}
#endif

#if APP_CAR_KEY_TEST_ENABLED
static void car_key_test_step(void)
{
    static bool running;

    running = !running;
    if (running) {
        CarChassis_SetTargetMmps(CAR_KEY_TEST_TARGET_A_MMPS,
                                 CAR_KEY_TEST_TARGET_B_MMPS);
        App_DebugLog("CAR,KEY,RUN,SLOW\r\n");
    } else {
        CarChassis_SetTargetMmps(0, 0);
        CarChassis_StopAll();
        App_DebugLog("CAR,KEY,STOP\r\n");
    }
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
            x42s_key_test_step(now_ms);
        }
#endif
#if APP_SERVO_KEY_TEST_ENABLED
        if (key_is_pressed) {
            servo_key_test_step();
        }
#endif
#if APP_CAR_KEY_TEST_ENABLED
        if (key_is_pressed) {
            car_key_test_step();
        }
#endif
    }

#if APP_X42S_KEY_TEST_ENABLED
    x42s_key_test_service(now_ms);
#endif
#if APP_SERVO_KEY_TEST_ENABLED
    servo_auto_sweep_service(now_ms);
#endif
#if APP_IPS200PRO_SCREEN_ENABLED
    ips200pro_time_service(now_ms);
#endif
#if APP_OLED_TIMER_SCREEN_ENABLED
    oled_timer_screen_service(now_ms);
#endif

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
