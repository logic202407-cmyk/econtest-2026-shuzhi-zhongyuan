#include "car_encoder.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"

static volatile int32_t g_encoder_a;
static volatile int32_t g_encoder_b;
static int32_t g_last_encoder_a;
static int32_t g_last_encoder_b;
static int32_t g_speed_mmps_a;
static int32_t g_speed_mmps_b;

static void encoder_a1_callback(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if (gpio_get_level(CAR_ENCODER_B1_PIN) != 0U) {
        g_encoder_a++;
    } else {
        g_encoder_a--;
    }
}

static void encoder_b1_callback(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if (gpio_get_level(CAR_ENCODER_A1_PIN) != 0U) {
        g_encoder_a--;
    } else {
        g_encoder_a++;
    }
}

static void encoder_a2_callback(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if (gpio_get_level(CAR_ENCODER_B2_PIN) != 0U) {
        g_encoder_b++;
    } else {
        g_encoder_b--;
    }
}

static void encoder_b2_callback(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if (gpio_get_level(CAR_ENCODER_A2_PIN) != 0U) {
        g_encoder_b--;
    } else {
        g_encoder_b++;
    }
}

void CarEncoder_Init(void)
{
    CarEncoder_Reset();
    exti_init(CAR_ENCODER_A1_PIN, EXTI_TRIGGER_BOTH, encoder_a1_callback, NULL);
    exti_init(CAR_ENCODER_B1_PIN, EXTI_TRIGGER_BOTH, encoder_b1_callback, NULL);
    exti_init(CAR_ENCODER_A2_PIN, EXTI_TRIGGER_BOTH, encoder_a2_callback, NULL);
    exti_init(CAR_ENCODER_B2_PIN, EXTI_TRIGGER_BOTH, encoder_b2_callback, NULL);
}

void CarEncoder_Reset(void)
{
    g_encoder_a = 0;
    g_encoder_b = 0;
    g_last_encoder_a = 0;
    g_last_encoder_b = 0;
    g_speed_mmps_a = 0;
    g_speed_mmps_b = 0;
}

void CarEncoder_UpdateSpeed(void)
{
    int32_t current_a;
    int32_t current_b;
    int32_t delta_a;
    int32_t delta_b;
    uint32 primask;

    primask = interrupt_global_disable();
    current_a = g_encoder_a;
    current_b = g_encoder_b;
    interrupt_global_enable(primask);

    delta_a = current_a - g_last_encoder_a;
    delta_b = current_b - g_last_encoder_b;

    g_speed_mmps_a = (int32_t)(((int64_t)delta_a *
        (int64_t)CAR_ENCODER_WHEEL_CIRCUM_X1000) /
        ((int64_t)CAR_ENCODER_PULSES_PER_REV * (int64_t)CAR_ENCODER_SAMPLE_MS));
    g_speed_mmps_b = (int32_t)(((int64_t)delta_b *
        (int64_t)CAR_ENCODER_WHEEL_CIRCUM_X1000) /
        ((int64_t)CAR_ENCODER_PULSES_PER_REV * (int64_t)CAR_ENCODER_SAMPLE_MS));

    g_last_encoder_a = current_a;
    g_last_encoder_b = current_b;
}

int32_t CarEncoder_GetCountA(void)
{
    return g_encoder_a;
}

int32_t CarEncoder_GetCountB(void)
{
    return g_encoder_b;
}

int32_t CarEncoder_GetSpeedMmpsA(void)
{
    return g_speed_mmps_a;
}

int32_t CarEncoder_GetSpeedMmpsB(void)
{
    return g_speed_mmps_b;
}
