#include "car_gyro.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"

#define CAR_GYRO_REG_DSP_CTL2       0x02U
#define CAR_GYRO_REG_OUT_CTL1       0x0BU
#define CAR_GYRO_CMD_SOFT_RESET     0x09U
#define CAR_GYRO_CMD_SLEEP_OUT      0x06U
#define CAR_GYRO_CMD_DATA_ACC       0x0AU
#define CAR_GYRO_DSP_CTL2_VALUE     0x20U
#define CAR_GYRO_LSB_TO_RAD_S       0.00024932f
#define CAR_GYRO_LPF_ALPHA          0.10f
#define CAR_GYRO_DEAD_ZONE_RAD_S    0.05f
#define CAR_GYRO_CALIB_SCALE        0.98888f
#define CAR_GYRO_PI                 3.14159265358979323846f

static soft_iic_info_struct g_car_gyro_iic;
static CarGyroStatus g_car_gyro_status;
static uint32_t g_car_gyro_deadline_ms;
static uint32_t g_car_gyro_last_sample_ms;
static uint32_t g_car_gyro_last_success_ms;
static uint16_t g_car_gyro_bias_count;
static float g_car_gyro_bias_sum;
static float g_car_gyro_bias_rad_s;
static float g_car_gyro_filtered_rad_s;
static float g_car_gyro_yaw_rad;
static uint8_t g_car_gyro_filter_valid;
static int32_t g_car_gyro_yaw_x100;
static int32_t g_car_gyro_wz_x100;

static uint8_t car_gyro_send(uint8_t data)
{
    return soft_iic_send_data(&g_car_gyro_iic, data);
}

static uint8_t car_gyro_send_command(uint8_t command)
{
    uint8_t ok;

    soft_iic_start(&g_car_gyro_iic);
    ok = car_gyro_send((uint8_t)(CAR_GYRO_I2C_ADDRESS << 1));
    if (ok != 0U) {
        ok = car_gyro_send(command);
    }
    soft_iic_stop(&g_car_gyro_iic);
    return ok;
}

static uint8_t car_gyro_write_register(uint8_t reg, uint8_t value)
{
    uint8_t ok;

    soft_iic_start(&g_car_gyro_iic);
    ok = car_gyro_send((uint8_t)(CAR_GYRO_I2C_ADDRESS << 1));
    if (ok != 0U) {
        ok = car_gyro_send(reg);
    }
    if (ok != 0U) {
        ok = car_gyro_send(value);
    }
    soft_iic_stop(&g_car_gyro_iic);
    return ok;
}

static uint8_t car_gyro_read_wz(float *wz_rad_s)
{
    uint8_t high;
    uint8_t low;
    uint8_t ok;
    int16_t raw;

    if (wz_rad_s == NULL) {
        return 0U;
    }

    soft_iic_start(&g_car_gyro_iic);
    ok = car_gyro_send((uint8_t)(CAR_GYRO_I2C_ADDRESS << 1));
    if (ok != 0U) {
        ok = car_gyro_send(CAR_GYRO_CMD_DATA_ACC);
    }
    if (ok != 0U) {
        soft_iic_start(&g_car_gyro_iic);
        ok = car_gyro_send((uint8_t)((CAR_GYRO_I2C_ADDRESS << 1) | 0x01U));
    }
    if (ok == 0U) {
        soft_iic_stop(&g_car_gyro_iic);
        return 0U;
    }

    high = soft_iic_read_data(&g_car_gyro_iic, 0U);
    low = soft_iic_read_data(&g_car_gyro_iic, 1U);
    soft_iic_stop(&g_car_gyro_iic);

    raw = (int16_t)(((uint16_t)high << 8) | low);
    *wz_rad_s = (float)raw * CAR_GYRO_LSB_TO_RAD_S;
    return 1U;
}

static void car_gyro_sample(uint32_t now_ms)
{
    float wz_rad_s;
    float compensated_rad_s;
    float dt_s;

    if (car_gyro_read_wz(&wz_rad_s) == 0U) {
        return;
    }

    if (g_car_gyro_filter_valid == 0U) {
        g_car_gyro_filtered_rad_s = wz_rad_s;
        g_car_gyro_filter_valid = 1U;
    } else {
        g_car_gyro_filtered_rad_s = CAR_GYRO_LPF_ALPHA * wz_rad_s +
            (1.0f - CAR_GYRO_LPF_ALPHA) * g_car_gyro_filtered_rad_s;
    }

    g_car_gyro_last_success_ms = now_ms;
    if (g_car_gyro_status == CAR_GYRO_STATUS_CALIBRATING) {
        g_car_gyro_bias_sum += g_car_gyro_filtered_rad_s;
        ++g_car_gyro_bias_count;
        if (g_car_gyro_bias_count >= CAR_GYRO_BIAS_SAMPLE_COUNT) {
            g_car_gyro_bias_rad_s = g_car_gyro_bias_sum /
                (float)g_car_gyro_bias_count;
            CarGyro_ResetYaw();
            g_car_gyro_status = CAR_GYRO_STATUS_READY;
        }
        return;
    }

    if (g_car_gyro_status != CAR_GYRO_STATUS_READY) {
        return;
    }

    compensated_rad_s = g_car_gyro_filtered_rad_s - g_car_gyro_bias_rad_s;
    if ((compensated_rad_s > -CAR_GYRO_DEAD_ZONE_RAD_S) &&
        (compensated_rad_s < CAR_GYRO_DEAD_ZONE_RAD_S)) {
        compensated_rad_s = 0.0f;
    }

    dt_s = (float)CAR_GYRO_SAMPLE_PERIOD_MS * 0.001f;
    g_car_gyro_yaw_rad += compensated_rad_s * dt_s * CAR_GYRO_CALIB_SCALE;
    while (g_car_gyro_yaw_rad > CAR_GYRO_PI) {
        g_car_gyro_yaw_rad -= 2.0f * CAR_GYRO_PI;
    }
    while (g_car_gyro_yaw_rad < -CAR_GYRO_PI) {
        g_car_gyro_yaw_rad += 2.0f * CAR_GYRO_PI;
    }

    g_car_gyro_yaw_x100 = (int32_t)(g_car_gyro_yaw_rad * 18000.0f / CAR_GYRO_PI);
    g_car_gyro_wz_x100 = (int32_t)(compensated_rad_s * 18000.0f / CAR_GYRO_PI);
}

void CarGyro_Init(uint32_t now_ms)
{
    soft_iic_init(&g_car_gyro_iic, CAR_GYRO_I2C_ADDRESS, 80U,
                  CAR_GYRO_SOFT_IIC_SCL_PIN, CAR_GYRO_SOFT_IIC_SDA_PIN);
    g_car_gyro_status = CAR_GYRO_STATUS_WAIT_RESET;
    g_car_gyro_deadline_ms = now_ms + 100U;
    g_car_gyro_last_sample_ms = now_ms;
    g_car_gyro_last_success_ms = now_ms;
    g_car_gyro_bias_count = 0U;
    g_car_gyro_bias_sum = 0.0f;
    g_car_gyro_bias_rad_s = 0.0f;
    g_car_gyro_filtered_rad_s = 0.0f;
    g_car_gyro_yaw_rad = 0.0f;
    g_car_gyro_filter_valid = 0U;
    g_car_gyro_yaw_x100 = 0;
    g_car_gyro_wz_x100 = 0;
}

void CarGyro_Service(uint32_t now_ms)
{
    if ((g_car_gyro_status == CAR_GYRO_STATUS_FAILED) ||
        (g_car_gyro_status == CAR_GYRO_STATUS_OFFLINE)) {
        return;
    }

    if ((g_car_gyro_status == CAR_GYRO_STATUS_WAIT_RESET) &&
        ((int32_t)(now_ms - g_car_gyro_deadline_ms) >= 0)) {
        if (car_gyro_send_command(CAR_GYRO_CMD_SOFT_RESET) == 0U) {
            g_car_gyro_status = CAR_GYRO_STATUS_FAILED;
            return;
        }
        g_car_gyro_status = CAR_GYRO_STATUS_WAIT_SLEEP_OUT;
        g_car_gyro_deadline_ms = now_ms + 50U;
        return;
    }

    if ((g_car_gyro_status == CAR_GYRO_STATUS_WAIT_SLEEP_OUT) &&
        ((int32_t)(now_ms - g_car_gyro_deadline_ms) >= 0)) {
        if (car_gyro_send_command(CAR_GYRO_CMD_SLEEP_OUT) == 0U) {
            g_car_gyro_status = CAR_GYRO_STATUS_FAILED;
            return;
        }
        g_car_gyro_status = CAR_GYRO_STATUS_WAIT_CONFIG;
        g_car_gyro_deadline_ms = now_ms + 200U;
        return;
    }

    if ((g_car_gyro_status == CAR_GYRO_STATUS_WAIT_CONFIG) &&
        ((int32_t)(now_ms - g_car_gyro_deadline_ms) >= 0)) {
        if ((car_gyro_write_register(CAR_GYRO_REG_OUT_CTL1, 0x01U) == 0U) ||
            (car_gyro_write_register(CAR_GYRO_REG_DSP_CTL2,
                                     CAR_GYRO_DSP_CTL2_VALUE) == 0U)) {
            g_car_gyro_status = CAR_GYRO_STATUS_FAILED;
            return;
        }
        g_car_gyro_status = CAR_GYRO_STATUS_CALIBRATING;
        g_car_gyro_bias_count = 0U;
        g_car_gyro_bias_sum = 0.0f;
        g_car_gyro_last_sample_ms = now_ms;
        return;
    }

    if (((g_car_gyro_status == CAR_GYRO_STATUS_CALIBRATING) ||
         (g_car_gyro_status == CAR_GYRO_STATUS_READY)) &&
        ((uint32_t)(now_ms - g_car_gyro_last_sample_ms) >=
         CAR_GYRO_SAMPLE_PERIOD_MS)) {
        g_car_gyro_last_sample_ms = now_ms;
        car_gyro_sample(now_ms);
    }
}

void CarGyro_ResetYaw(void)
{
    g_car_gyro_yaw_rad = 0.0f;
    g_car_gyro_yaw_x100 = 0;
    g_car_gyro_wz_x100 = 0;
}

CarGyroStatus CarGyro_GetStatus(void)
{
    return g_car_gyro_status;
}

uint8_t CarGyro_IsReady(void)
{
    return (g_car_gyro_status == CAR_GYRO_STATUS_READY) ? 1U : 0U;
}

int32_t CarGyro_GetYawX100(void)
{
    return g_car_gyro_yaw_x100;
}

int32_t CarGyro_GetWzX100(void)
{
    return g_car_gyro_wz_x100;
}

uint32_t CarGyro_GetDataAgeMs(uint32_t now_ms)
{
    return now_ms - g_car_gyro_last_success_ms;
}
