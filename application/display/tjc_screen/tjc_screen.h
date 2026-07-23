#ifndef TJC_SCREEN_H
#define TJC_SCREEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void TJC_ScreenInit(void);
void TJC_SendCommand(const char *command);
void TJC_SetBrightness(uint8_t percent);
void TJC_SetText(const char *object_name, const char *text);
void TJC_SetNumber(const char *object_name, int32_t value);
void TJC_ShowBasicStatus(const char *mode, const char *vision, int32_t yaw_0p01deg,
                         int32_t pitch_0p01deg, const char *motor);

#ifdef __cplusplus
}
#endif

#endif
