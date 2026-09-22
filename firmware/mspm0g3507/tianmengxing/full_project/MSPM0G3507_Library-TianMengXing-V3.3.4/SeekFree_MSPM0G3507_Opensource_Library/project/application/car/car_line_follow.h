#ifndef CAR_LINE_FOLLOW_H
#define CAR_LINE_FOLLOW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_LINE_CORRECTION_DIVISOR     2
#define CAR_LINE_CORRECTION_LIMIT_MMPS  220
#define CAR_LINE_TURN_SIGN              1
#define CAR_LINE_A_FORWARD_SIGN         1
#define CAR_LINE_B_FORWARD_SIGN         1
#define CAR_LINE_CURVE_SLOWDOWN_DIVISOR 2
#define CAR_LINE_CURVE_SLOWDOWN_MAX_MMPS 110
#define CAR_LINE_MIN_WHEEL_SPEED_MMPS   60
// The previous state machine was written for a right-angle marker. A smooth
// half-circle must remain in continuous differential steering instead.
#define CAR_LINE_SPECIAL_RIGHT_TURN_ENABLED 0U
#define CAR_LINE_RIGHT_TURN_LEFT_MMPS   200
#define CAR_LINE_RIGHT_TURN_DELAY_MS    400U

void CarLineFollow_Init(void);
void CarLineFollow_Update(void);
int16_t CarLineFollow_GetCorrection(void);
int32_t CarLineFollow_GetCorrectionMmps(void);
int32_t CarLineFollow_ApplyCorrectionA(int32_t base_target_a);
int32_t CarLineFollow_ApplyCorrectionB(int32_t base_target_b);
uint8_t CarLineFollow_IsRightTurning(void);

#ifdef __cplusplus
}
#endif

#endif
