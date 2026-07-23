#ifndef CAR_CHASSIS_H
#define CAR_CHASSIS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void CarChassis_Init(void);
void CarChassis_SetMotorA(int16_t speed);
void CarChassis_SetMotorB(int16_t speed);
void CarChassis_StopAll(void);
void CarChassis_ControlTick1ms(void);
void CarChassis_Service50ms(void);
uint8_t CarChassis_TakeSpeedUpdateFlag(void);
void CarChassis_SetTargetMmps(int32_t target_a, int32_t target_b);
int32_t CarChassis_GetActualMmpsA(void);
int32_t CarChassis_GetActualMmpsB(void);
int32_t CarChassis_GetOutputA(void);
int32_t CarChassis_GetOutputB(void);

#ifdef __cplusplus
}
#endif

#endif
