#ifndef SKYSTAR_KEY_H
#define SKYSTAR_KEY_H

#include <stdint.h>

void SkystarKey_Init(void);
uint8_t SkystarKey_IsPressed(void);
uint8_t SkystarKey_Update(uint32_t now_ms);

#endif
