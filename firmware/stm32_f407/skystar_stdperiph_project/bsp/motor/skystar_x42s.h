#ifndef SKYSTAR_X42S_H
#define SKYSTAR_X42S_H

#include <stdint.h>

#define SKYSTAR_X42S_DEFAULT_ID 1U

void SkystarX42S_Init(void);
void SkystarX42S_Enable(uint8_t id);
void SkystarX42S_Disable(uint8_t id);
void SkystarX42S_Stop(uint8_t id);
void SkystarX42S_ReadPosition(uint8_t id);
void SkystarX42S_SetPosition(uint8_t id, int32_t position_0p1deg);
void SkystarX42S_SetSpeed(uint8_t id, int32_t speed_0p1rpm);
void SkystarX42S_OnRxByte(uint8_t byte);
int32_t SkystarX42S_GetLastPosition(uint8_t id);

#endif
