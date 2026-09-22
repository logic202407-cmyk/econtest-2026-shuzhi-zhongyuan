#ifndef SKYSTAR_TJC_H
#define SKYSTAR_TJC_H

#include "stm32f4xx.h"
#include <stdint.h>

#define SKYSTAR_TJC_ENABLED        0U
#define SKYSTAR_TJC_BAUDRATE       115200U
#define SKYSTAR_TJC_UART_TX_LABEL  "C6"
#define SKYSTAR_TJC_UART_RX_LABEL  "C7"

void SkystarTjc_Init(void);
void SkystarTjc_SendCommand(const char *command);
void SkystarTjc_SetBrightness(uint8_t percent);
void SkystarTjc_SetText(const char *object_name, const char *text);
void SkystarTjc_SetNumber(const char *object_name, int32_t value);
void SkystarTjc_ShowBasicStatus(const char *mode,
                                int32_t yaw_0p1deg,
                                int32_t pitch_0p1deg,
                                uint8_t vision_ok);

#endif
