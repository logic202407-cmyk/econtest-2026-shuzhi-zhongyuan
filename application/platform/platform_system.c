#include "platform_system.h"

#include <stdio.h>
#include <string.h>

#include "zf_common_headfile.h"

#include "../config/app_config.h"
#include "platform_gpio.h"
#include "platform_uart.h"
#include "../car/car_chassis.h"
#include "../car/car_encoder.h"
#if CAR_GYRO_ENABLED
#include "../car/car_gyro.h"
#endif
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
#if APP_MATCH_TIMER_ENABLED
static void match_timer_init(void);
static void match_timer_key_step(uint32_t now_ms);
#endif
#if APP_CAR_KEY_TEST_ENABLED
static void car_key_test_step(void);
#endif
#if APP_CAR_ENCODER_TEST_ENABLED
static void car_encoder_test_init(void);
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
#if APP_MATCH_TIMER_ENABLED
    match_timer_init();
#endif
#if APP_CAR_ENCODER_TEST_ENABLED
    car_encoder_test_init();
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

#if APP_MATCH_TIMER_ENABLED
typedef enum
{
    MATCH_TIMER_READY = 0,
    MATCH_TIMER_RUNNING,
    MATCH_TIMER_STOPPED
} MatchTimerState;

static MatchTimerState g_match_timer_state;
static uint32_t g_match_timer_start_ms;
static uint32_t g_match_timer_elapsed_ms;
#endif

#if APP_OLED_TIMER_SCREEN_ENABLED
static uint32_t g_oled_timer_last_show_ms;
static uint32_t g_oled_timer_last_centiseconds = 0xFFFFFFFFUL;
static MatchTimerState g_oled_timer_last_state = (MatchTimerState)0xFFU;

static const char *match_timer_status_text(void)
{
    switch (g_match_timer_state) {
    case MATCH_TIMER_RUNNING:
        return "RUN";
    case MATCH_TIMER_STOPPED:
        return "STOP";
    default:
        return "READY";
    }
}

static uint32_t match_timer_elapsed_ms(uint32_t now_ms)
{
    if (g_match_timer_state == MATCH_TIMER_RUNNING) {
        return now_ms - g_match_timer_start_ms;
    }
    return g_match_timer_elapsed_ms;
}

static void oled_timer_screen_show(uint32_t now_ms)
{
    uint32_t elapsed_ms = match_timer_elapsed_ms(now_ms);
    uint32_t total_seconds = elapsed_ms / 1000U;
    uint32_t minutes = (total_seconds / 60U) % 100U;
    uint32_t seconds = total_seconds % 60U;
    uint32_t centiseconds = (elapsed_ms % 1000U) / 10U;
    char large_time[6];
    char precise_time[12];

    // The OLED is cleared only during initialization. Rewriting the small
    // status band prevents stale letters without visible full-screen flicker.
    OledI2c_ShowString(0U, 0U, "                     ");
    (void)snprintf(large_time, sizeof(large_time), "%02lu:%02lu",
                   (unsigned long)minutes, (unsigned long)seconds);
    (void)snprintf(precise_time, sizeof(precise_time), "%02lu:%02lu.%02lu",
                   (unsigned long)minutes, (unsigned long)seconds,
                   (unsigned long)centiseconds);
    OledI2c_ShowString(48U, 0U, match_timer_status_text());
    OledI2c_ShowLargeString(19U, 2U, large_time, 3U);
    OledI2c_ShowString(34U, 6U, precise_time);
}

static void oled_timer_screen_init(void)
{
    OledI2c_Init();
    g_oled_timer_last_show_ms = 0xFFFFFFFFUL;
    g_oled_timer_last_centiseconds = 0xFFFFFFFFUL;
    g_oled_timer_last_state = (MatchTimerState)0xFFU;
    oled_timer_screen_show(App_GetMillis());
    App_DebugLog("OLED,MATCH_TIMER,INIT,A31-SCL,A28-SDA\r\n");
}

static void oled_timer_screen_service(uint32_t now_ms)
{
    uint32_t centiseconds;

    if ((uint32_t)(now_ms - g_oled_timer_last_show_ms) <
        APP_MATCH_TIMER_DISPLAY_MS) {
        return;
    }

    g_oled_timer_last_show_ms = now_ms;
    centiseconds = match_timer_elapsed_ms(now_ms) / 10U;
    if (g_oled_timer_last_state == g_match_timer_state &&
        g_oled_timer_last_centiseconds == centiseconds) {
        return;
    }

    g_oled_timer_last_state = g_match_timer_state;
    g_oled_timer_last_centiseconds = centiseconds;
    oled_timer_screen_show(now_ms);
}
#endif

#if APP_MATCH_TIMER_ENABLED
static void match_timer_init(void)
{
    g_match_timer_state = MATCH_TIMER_READY;
    g_match_timer_start_ms = 0U;
    g_match_timer_elapsed_ms = 0U;
}

static void match_timer_key_step(uint32_t now_ms)
{
    switch (g_match_timer_state) {
    case MATCH_TIMER_READY:
        g_match_timer_start_ms = now_ms;
        g_match_timer_elapsed_ms = 0U;
        g_match_timer_state = MATCH_TIMER_RUNNING;
#if CAR_MOTION_ENABLED
        CarChassis_SetControlEnabled(1U);
        CarChassis_SetTargetMmps(CAR_KEY_TEST_TARGET_A_MMPS,
                                 CAR_KEY_TEST_TARGET_B_MMPS);
#endif
        App_DebugLog("MATCH,START\r\n");
        break;

    case MATCH_TIMER_RUNNING:
        g_match_timer_elapsed_ms = now_ms - g_match_timer_start_ms;
        g_match_timer_state = MATCH_TIMER_STOPPED;
#if CAR_MOTION_ENABLED
        CarChassis_SetControlEnabled(0U);
#endif
        App_DebugLog("MATCH,STOP\r\n");
        break;

    default:
        g_match_timer_elapsed_ms = 0U;
        g_match_timer_state = MATCH_TIMER_READY;
        App_DebugLog("MATCH,RESET\r\n");
        break;
    }

#if APP_OLED_TIMER_SCREEN_ENABLED
    g_oled_timer_last_show_ms = now_ms - APP_MATCH_TIMER_DISPLAY_MS;
    oled_timer_screen_service(now_ms);
#endif
}
#endif

#if APP_CAR_ENCODER_TEST_ENABLED
static void car_encoder_test_init(void)
{
    CarEncoder_Init();
    App_DebugLog("CAR,ENCODER,TEST,INIT,A14,A15,A24,A17\r\n");
}
#endif

#if APP_CAR_KEY_TEST_ENABLED
static void car_key_test_step(void)
{
    static bool running;

    running = !running;
    if (running) {
        CarChassis_SetControlEnabled(1U);
        CarChassis_SetTargetMmps(CAR_KEY_TEST_TARGET_A_MMPS,
                                 CAR_KEY_TEST_TARGET_B_MMPS);
        App_DebugLog("CAR,KEY,RUN,SLOW\r\n");
    } else {
        CarChassis_SetControlEnabled(0U);
        App_DebugLog("CAR,KEY,STOP\r\n");
    }
}
#endif

void App_BoardService(uint32_t now_ms)
{
#if APP_BOARD_SELF_TEST_ENABLED
    static uint32_t last_led_toggle_ms;
    static uint32_t last_vision_tx_ms;
    static uint32_t key_released_since_ms;
    static bool key_release_seen;
    static bool key_was_pressed;
    static bool key_armed;

    if ((now_ms - last_led_toggle_ms) >= APP_BOARD_LED_HEARTBEAT_MS) {
        last_led_toggle_ms = now_ms;
        platform_led_toggle();
    }

    bool key_is_pressed = platform_key_read();

    // A low level while B21 is still settling at power-up must never start a
    // competition run. Arm the key only after observing its released state.
    if (!key_armed) {
        if (key_is_pressed) {
            key_release_seen = false;
        } else if (!key_release_seen) {
            key_release_seen = true;
            key_released_since_ms = now_ms;
        } else if ((uint32_t)(now_ms - key_released_since_ms) >=
                   APP_MATCH_KEY_ARM_MS) {
            key_armed = true;
            App_DebugLog("KEY,ARMED,B21\r\n");
        }
        return;
    }

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
#if APP_MATCH_TIMER_ENABLED
        if (key_is_pressed) {
            match_timer_key_step(now_ms);
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
