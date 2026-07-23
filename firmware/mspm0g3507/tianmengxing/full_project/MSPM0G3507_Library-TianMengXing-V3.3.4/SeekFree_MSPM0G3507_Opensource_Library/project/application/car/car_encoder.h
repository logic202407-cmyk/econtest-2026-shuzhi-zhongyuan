#ifndef CAR_ENCODER_H
#define CAR_ENCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void CarEncoder_Init(void);
void CarEncoder_Reset(void);
void CarEncoder_UpdateSpeed(void);
int32_t CarEncoder_GetCountA(void);
int32_t CarEncoder_GetCountB(void);
int32_t CarEncoder_GetSpeedMmpsA(void);
int32_t CarEncoder_GetSpeedMmpsB(void);

#ifdef __cplusplus
}
#endif

#endif
