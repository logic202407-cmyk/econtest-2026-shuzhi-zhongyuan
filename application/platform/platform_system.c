#include "platform_system.h"

#include "zf_common_headfile.h"

#include "platform_gpio.h"

static volatile uint32_t g_platform_tick_ms;

static void platform_tick_handler(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;
    ++g_platform_tick_ms;
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

uint32_t App_GetMillis(void)
{
    return platform_time_ms();
}
