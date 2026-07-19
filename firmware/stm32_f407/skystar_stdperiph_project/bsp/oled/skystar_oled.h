#ifndef SKYSTAR_OLED_H
#define SKYSTAR_OLED_H

#include <stdint.h>

#define SKYSTAR_OLED_COLUMNS 21U
#define SKYSTAR_OLED_ROWS    4U

void SkystarOled_Init(void);
void SkystarOled_Clear(void);
void SkystarOled_ShowLine(uint8_t row, const char *text);

#endif
