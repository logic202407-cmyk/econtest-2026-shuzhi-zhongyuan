#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Platform_GpioInit(void);
void Platform_LedSet(bool on);
void Platform_LedToggle(void);
bool Platform_KeyIsPressed(void);
void Platform_Rs485SetTxEnable(bool enable);

#ifdef __cplusplus
}
#endif

#endif
