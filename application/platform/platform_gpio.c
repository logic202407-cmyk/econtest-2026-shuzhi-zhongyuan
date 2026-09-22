#include "platform_gpio.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"

void platform_gpio_init(void)
{
    gpio_init(LED_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
    gpio_init(KEY_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(APP_EXPANSION_KEY1_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(APP_EXPANSION_KEY2_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(RS485_DE_PIN, GPO, RS485_DE_RX_LEVEL, GPO_PUSH_PULL);
}

void platform_led_set(bool on)
{
    gpio_set_level(LED_PIN, on ? GPIO_HIGH : GPIO_LOW);
}

void platform_led_toggle(void)
{
    gpio_toggle_level(LED_PIN);
}

bool platform_key_read(void)
{
    return gpio_get_level(KEY_PIN) == GPIO_LOW;
}

bool App_HBalanceArmKeyPressed(void)
{
    return gpio_get_level(APP_EXPANSION_KEY1_PIN) == GPIO_LOW;
}

void platform_rs485_set_tx_enable(bool enable)
{
    gpio_set_level(RS485_DE_PIN,
                   enable ? RS485_DE_TX_LEVEL : RS485_DE_RX_LEVEL);
}

void App_Rs485SetTxEnable(bool enable)
{
    platform_rs485_set_tx_enable(enable);
}
