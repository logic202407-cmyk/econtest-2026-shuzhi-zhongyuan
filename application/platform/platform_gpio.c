#include "platform_gpio.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"

void Platform_GpioInit(void)
{
    gpio_init(LED_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
    gpio_init(KEY_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(RS485_DE_PIN, GPO, RS485_DE_RX_LEVEL, GPO_PUSH_PULL);
}

void Platform_LedSet(bool on)
{
    gpio_set_level(LED_PIN, on ? GPIO_HIGH : GPIO_LOW);
}

void Platform_LedToggle(void)
{
    gpio_toggle_level(LED_PIN);
}

bool Platform_KeyIsPressed(void)
{
    return gpio_get_level(KEY_PIN) == GPIO_LOW;
}

void Platform_Rs485SetTxEnable(bool enable)
{
    gpio_set_level(RS485_DE_PIN,
                   enable ? RS485_DE_TX_LEVEL : RS485_DE_RX_LEVEL);
}

void App_SystemInit(void)
{
    Platform_GpioInit();
}

void App_Rs485SetTxEnable(bool enable)
{
    Platform_Rs485SetTxEnable(enable);
}
