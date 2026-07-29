#ifndef CAR_GYRO_H
#define CAR_GYRO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CAR_GYRO_STATUS_OFFLINE = 0U,
    CAR_GYRO_STATUS_WAIT_RESET,
    CAR_GYRO_STATUS_WAIT_SLEEP_OUT,
    CAR_GYRO_STATUS_WAIT_CONFIG,
    CAR_GYRO_STATUS_CALIBRATING,
    CAR_GYRO_STATUS_READY,
    CAR_GYRO_STATUS_FAILED
} CarGyroStatus;

void CarGyro_Init(uint32_t now_ms);
void CarGyro_Service(uint32_t now_ms);
void CarGyro_ResetYaw(void);
CarGyroStatus CarGyro_GetStatus(void);
uint8_t CarGyro_IsReady(void);
int32_t CarGyro_GetYawX100(void);
int32_t CarGyro_GetWzX100(void);
uint32_t CarGyro_GetDataAgeMs(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
