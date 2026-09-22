#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void platform_gpio_init(void);
void platform_led_set(bool on);
void platform_led_toggle(void);
bool platform_key_read(void);
bool App_HBalanceArmKeyPressed(void);
void platform_rs485_set_tx_enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif
