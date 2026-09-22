#include "car_gray.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"

uint8_t car_gray_value[CAR_GRAY_SENSOR_NUM] = {
    0U, 0U, 0U, 0U, 0U, 0U, 0U
};

int16_t g_car_gray_correction = 0;

static const gpio_pin_enum g_gray_pins[CAR_GRAY_SENSOR_NUM] = {
    CAR_GRAY_OUT1_PIN,
    CAR_GRAY_OUT2_PIN,
    CAR_GRAY_OUT3_PIN,
    CAR_GRAY_OUT4_PIN,
    CAR_GRAY_OUT5_PIN,
    CAR_GRAY_OUT6_PIN,
    CAR_GRAY_OUT7_PIN
};

static const int16_t g_gray_weight[CAR_GRAY_SENSOR_NUM] = {
    -240,
    -160,
     -60,
       0,
      60,
     160,
     240
};

void CarGray_Init(void)
{
    uint8_t i;

    for (i = 0U; i < CAR_GRAY_SENSOR_NUM; ++i) {
        gpio_init(g_gray_pins[i], GPI, GPIO_HIGH, GPI_PULL_UP);
    }
}

void CarGray_Read(void)
{
    uint8_t i;

    for (i = 0U; i < CAR_GRAY_SENSOR_NUM; ++i) {
        car_gray_value[i] = (gpio_get_level(g_gray_pins[i]) != 0U) ? 1U : 0U;
    }
}

uint8_t CarGray_GetValue(uint8_t index)
{
    if (index >= CAR_GRAY_SENSOR_NUM) {
        return 0U;
    }

    return car_gray_value[index];
}

uint8_t CarGray_GetBits(void)
{
    uint8_t bits = 0U;
    uint8_t i;

    for (i = 0U; i < CAR_GRAY_SENSOR_NUM; ++i) {
        if (car_gray_value[i] != 0U) {
            bits |= (uint8_t)(1U << (6U - i));
        }
    }

    return bits;
}

int16_t CarGray_CalculateCorrection(void)
{
    uint8_t i;
    int32_t correction = 0;

    CarGray_Read();
    for (i = 0U; i < CAR_GRAY_SENSOR_NUM; ++i) {
        correction += (int32_t)car_gray_value[i] * (int32_t)g_gray_weight[i];
    }

    g_car_gray_correction = (int16_t)correction;
    return g_car_gray_correction;
}
