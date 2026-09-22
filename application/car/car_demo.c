#include "car_demo.h"

#include <stdio.h>
#include <stdint.h>

#include "../config/app_config.h"
#include "../display/oled_i2c/oled_i2c.h"
#include "../platform/platform_gpio.h"
#include "car_chassis.h"
#include "car_gray.h"
#include "car_line_follow.h"

void App_SystemInit(void);
void App_PlatformInit(void);
void App_DebugUartInit(void);
void App_DebugLog(const char *message);
uint32_t App_GetMillis(void);
void App_Idle(void);

static void append_gray_bits(char *buffer, uint32_t size)
{
    uint8_t i;
    uint32_t offset = 0U;

    if (size == 0U) {
        return;
    }

    for (i = 0U; (i < CAR_GRAY_SENSOR_COUNT) && (offset + 1U < size); ++i) {
        buffer[offset++] = (char)('0' + CarGray_GetValue(i));
    }
    buffer[offset] = '\0';
}

static void log_car_status(void)
{
    char gray[8];
    char line[128];

    append_gray_bits(gray, sizeof(gray));
    snprintf(line, sizeof(line),
             "CAR,A=%ld,B=%ld,OA=%ld,OB=%ld,C=%d,G=%s\r\n",
             (long)CarChassis_GetActualMmpsA(),
             (long)CarChassis_GetActualMmpsB(),
             (long)CarChassis_GetOutputA(),
             (long)CarChassis_GetOutputB(),
             (int)CarLineFollow_GetCorrection(),
             gray);
    App_DebugLog(line);
}

static void display_car_status(void)
{
    char gray[8];

    append_gray_bits(gray, sizeof(gray));
    OledI2c_ShowString(0U, 0U, "MM/S C:");
    OledI2c_ShowInt(48U, 0U, (int32_t)CarLineFollow_GetCorrection(), 4U);
    OledI2c_ShowString(0U, 2U, "A:");
    OledI2c_ShowInt(18U, 2U, CarChassis_GetActualMmpsA(), 5U);
    OledI2c_ShowString(0U, 4U, "B:");
    OledI2c_ShowInt(18U, 4U, CarChassis_GetActualMmpsB(), 5U);
    OledI2c_ShowString(0U, 6U, "G:");
    OledI2c_ShowString(18U, 6U, gray);
}

void CarDemo_Run(void)
{
    App_SystemInit();
    App_PlatformInit();
    App_DebugUartInit();

    CarGray_Init();
    CarChassis_Init();
    OledI2c_Init();
    CarChassis_SetTargetMmps(CAR_MOTOR_DEFAULT_TARGET_A_MMPS,
                             CAR_MOTOR_DEFAULT_TARGET_B_MMPS);

    App_DebugLog("CAR,INIT,TianMengXing\r\n");
    App_DebugLog("CAR,PINS,MOTOR=A12/A13,B15/A27,ENC=A14/A15,A24/A17,GRAY=B25/B24/B20/B18/B19/B10/A7\r\n");
    OledI2c_ShowString(0U, 0U, "CAR INIT");

    while (1) {
        if (CarChassis_TakeSpeedUpdateFlag() != 0U) {
            platform_led_toggle();
            display_car_status();
            log_car_status();
        }

        App_Idle();
    }
}
