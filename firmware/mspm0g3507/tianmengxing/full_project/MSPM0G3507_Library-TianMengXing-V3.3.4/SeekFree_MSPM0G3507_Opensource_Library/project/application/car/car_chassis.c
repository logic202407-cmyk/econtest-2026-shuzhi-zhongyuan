#include "car_chassis.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"
#include "car_encoder.h"
#include "car_line_follow.h"
#include "car_pid.h"

#define CAR_PID_KP 0.7f
#define CAR_PID_KI 0.36f
#define CAR_PID_KD 0.0f
#define CAR_PID_INTEGRAL_MIN (-6000)
#define CAR_PID_INTEGRAL_MAX 6000

static CarPid g_pid_a;
static CarPid g_pid_b;
static int32_t g_target_a = CAR_MOTOR_DEFAULT_TARGET_A_MMPS;
static int32_t g_target_b = CAR_MOTOR_DEFAULT_TARGET_B_MMPS;
static int32_t g_feedback_a;
static int32_t g_feedback_b;
static int32_t g_output_a;
static int32_t g_output_b;
static volatile uint8_t g_speed_updated;
static volatile uint8_t g_control_enabled;
static volatile uint8_t g_control_tick_ms;
static volatile uint8_t g_chassis_initialized;

static uint16_t limit_duty(int16_t speed)
{
    int32_t duty = speed;

    if (duty < 0) {
        duty = -duty;
    }
    if (duty > (int32_t)CAR_MOTOR_PWM_MAX) {
        duty = (int32_t)CAR_MOTOR_PWM_MAX;
    }

    return (uint16_t)duty;
}

static void configure_pwm_channel(GPTIMER_Regs *timer, uint32_t channel)
{
    DL_TimerG_setCaptureCompareOutCtl(timer,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        channel);
    DL_TimerG_setCaptCompUpdateMethod(timer,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, channel);
    // A PWM channel must come up at 0 duty. Starting at the period value can
    // generate a full-duty pulse before CarChassis_StopAll() runs.
    DL_TimerG_setCaptureCompareValue(timer, 0U, channel);
}

static void configure_pwm_timer(GPTIMER_Regs *timer, pwm_channel_enum pin0,
                                pwm_channel_enum pin1)
{
    // This is the same TimerG setup generated in the teammate's verified
    // SysConfig project: 32 MHz / 8 / 40 = 100 kHz, period 1000 -> 100 Hz.
    static const DL_TimerG_ClockConfig clock_config = {
        .clockSel = DL_TIMER_CLOCK_BUSCLK,
        .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
        .prescale = 39U
    };
    static const DL_TimerG_PWMConfig pwm_config = {
        .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
        .period = CAR_MOTOR_PWM_MAX,
        .isTimerWithFourCC = true,
        .startTimer = DL_TIMER_STOP
    };

    DL_TimerG_reset(timer);
    DL_TimerG_enablePower(timer);
    delay_cycles(16U);
    // Decode the SeekFree PWM descriptors instead of hard-coding PINCM
    // indexes. This keeps the timer setup aligned with the selected pins.
    afio_init((gpio_pin_enum)(pin0 & PWM_PIN_INDEX_MASK), GPO,
              (gpio_af_enum)((pin0 >> PWM_PIN_AF_OFFSET) & PWM_PIN_AF_MASK),
              GPO_AF_PUSH_PULL);
    afio_init((gpio_pin_enum)(pin1 & PWM_PIN_INDEX_MASK), GPO,
              (gpio_af_enum)((pin1 >> PWM_PIN_AF_OFFSET) & PWM_PIN_AF_MASK),
              GPO_AF_PUSH_PULL);
    DL_TimerG_setClockConfig(timer, (DL_TimerG_ClockConfig *)&clock_config);
    DL_TimerG_initPWMMode(timer, (DL_TimerG_PWMConfig *)&pwm_config);
    configure_pwm_channel(timer, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    configure_pwm_channel(timer, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    DL_TimerG_enableClock(timer);
    DL_TimerG_setCCPDirection(timer, DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT);
    DL_TimerG_startCounter(timer);
}

void CarChassis_Init(void)
{
    CarEncoder_Init();
    CarLineFollow_Init();
    CarPid_Init(&g_pid_a, CAR_PID_KP, CAR_PID_KI, CAR_PID_KD,
                -(int32_t)CAR_MOTOR_PWM_MAX, (int32_t)CAR_MOTOR_PWM_MAX,
                CAR_PID_INTEGRAL_MIN, CAR_PID_INTEGRAL_MAX);
    CarPid_Init(&g_pid_b, CAR_PID_KP, CAR_PID_KI, CAR_PID_KD,
                -(int32_t)CAR_MOTOR_PWM_MAX, (int32_t)CAR_MOTOR_PWM_MAX,
                CAR_PID_INTEGRAL_MIN, CAR_PID_INTEGRAL_MAX);

    configure_pwm_timer(TIMG0, CAR_MOTOR_A_IN1_PWM_PIN,
                        CAR_MOTOR_A_IN2_PWM_PIN);
    configure_pwm_timer(TIMG7, CAR_MOTOR_B_IN1_PWM_PIN,
                        CAR_MOTOR_B_IN2_PWM_PIN);
    CarChassis_StopAll();
    g_speed_updated = 0U;
    g_control_tick_ms = 0U;
    // Do not let encoder or grayscale noise drive the wheels at power-on.
    g_control_enabled = 0U;
    g_chassis_initialized = 1U;
}

void CarChassis_SetMotorA(int16_t speed)
{
    uint16_t duty = limit_duty(speed);

    if (speed > 0) {
        DL_TimerG_setCaptureCompareValue(TIMG0, duty, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG0, 0U, DL_TIMER_CC_1_INDEX);
    } else if (speed < 0) {
        DL_TimerG_setCaptureCompareValue(TIMG0, 0U, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG0, duty, DL_TIMER_CC_1_INDEX);
    } else {
        DL_TimerG_setCaptureCompareValue(TIMG0, 0U, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG0, 0U, DL_TIMER_CC_1_INDEX);
    }
}

void CarChassis_SetMotorB(int16_t speed)
{
    uint16_t duty = limit_duty(speed);

    if (speed > 0) {
        DL_TimerG_setCaptureCompareValue(TIMG7, duty, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG7, 0U, DL_TIMER_CC_1_INDEX);
    } else if (speed < 0) {
        DL_TimerG_setCaptureCompareValue(TIMG7, 0U, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG7, duty, DL_TIMER_CC_1_INDEX);
    } else {
        DL_TimerG_setCaptureCompareValue(TIMG7, 0U, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(TIMG7, 0U, DL_TIMER_CC_1_INDEX);
    }
}

void CarChassis_ControlTick1ms(void)
{
    if (g_control_enabled == 0U) {
        if (g_chassis_initialized != 0U) {
            CarChassis_StopAll();
        }
        return;
    }

    ++g_control_tick_ms;
    if (g_control_tick_ms >= CAR_ENCODER_SAMPLE_MS) {
        g_control_tick_ms = 0U;
        CarChassis_Service50ms();
    }
}

void CarChassis_StopAll(void)
{
    CarChassis_SetMotorA(0);
    CarChassis_SetMotorB(0);
}

void CarChassis_SetControlEnabled(uint8_t enabled)
{
    if (enabled == 0U) {
        g_control_enabled = 0U;
        g_control_tick_ms = 0U;
        CarChassis_SetTargetMmps(0, 0);
        CarPid_Reset(&g_pid_a);
        CarPid_Reset(&g_pid_b);
        CarChassis_StopAll();
    } else {
        g_control_tick_ms = 0U;
        g_control_enabled = 1U;
    }
}

void CarChassis_Service50ms(void)
{
    int32_t target_a;
    int32_t target_b;

    CarEncoder_UpdateSpeed();
    g_feedback_a = CarEncoder_GetSpeedMmpsA();
    g_feedback_b = CarEncoder_GetSpeedMmpsB();

#if CAR_LINE_FOLLOW_ENABLED
    CarLineFollow_Update();
    target_a = CarLineFollow_ApplyCorrectionA(g_target_a);
    target_b = CarLineFollow_ApplyCorrectionB(g_target_b);
#else
    target_a = g_target_a;
    target_b = g_target_b;
#endif

    g_output_a = CarPid_UpdateIncremental(&g_pid_a, target_a, g_feedback_a);

#if CAR_LINE_FOLLOW_ENABLED
    if (CarLineFollow_IsRightTurning() != 0U) {
        g_output_b = 0;
        CarPid_Reset(&g_pid_b);
        CarChassis_SetMotorB(0);
    } else {
        g_output_b = CarPid_UpdateIncremental(&g_pid_b, target_b, g_feedback_b);
        CarChassis_SetMotorB((int16_t)g_output_b);
    }
#else
    g_output_b = CarPid_UpdateIncremental(&g_pid_b, target_b, g_feedback_b);
    CarChassis_SetMotorB((int16_t)g_output_b);
#endif

    CarChassis_SetMotorA((int16_t)g_output_a);
    g_speed_updated = 1U;
}

uint8_t CarChassis_TakeSpeedUpdateFlag(void)
{
    if (g_speed_updated == 0U) {
        return 0U;
    }

    g_speed_updated = 0U;
    return 1U;
}

void CarChassis_SetTargetMmps(int32_t target_a, int32_t target_b)
{
    g_target_a = target_a;
    g_target_b = target_b;
}

int32_t CarChassis_GetOutputA(void)
{
    return g_output_a;
}

int32_t CarChassis_GetOutputB(void)
{
    return g_output_b;
}

int32_t CarChassis_GetActualMmpsA(void)
{
    return g_feedback_a;
}

int32_t CarChassis_GetActualMmpsB(void)
{
    return g_feedback_b;
}
