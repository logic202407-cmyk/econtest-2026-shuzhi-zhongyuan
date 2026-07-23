#ifndef CAR_GRAY_H
#define CAR_GRAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_GRAY_SENSOR_NUM 7U

extern uint8_t car_gray_value[CAR_GRAY_SENSOR_NUM];
extern int16_t g_car_gray_correction;

void CarGray_Init(void);
void CarGray_Read(void);
uint8_t CarGray_GetValue(uint8_t index);
uint8_t CarGray_GetBits(void);
int16_t CarGray_CalculateCorrection(void);

#ifdef __cplusplus
}
#endif

#endif
