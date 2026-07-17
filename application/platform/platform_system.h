#ifndef PLATFORM_SYSTEM_H
#define PLATFORM_SYSTEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void platform_system_init(void);
void platform_time_init(void);
uint32_t platform_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif
