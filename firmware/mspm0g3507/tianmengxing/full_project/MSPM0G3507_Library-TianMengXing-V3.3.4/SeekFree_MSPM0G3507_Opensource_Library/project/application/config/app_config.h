#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Central hardware allocation for the TianMengXing MSPM0G3507 application.
// Application code should include this file instead of spelling raw pins.
#define APP_PIN_UNASSIGNED      0

// UART peripherals requested by the application architecture.
#define APP_DEBUG_UART          UART0
#define APP_VISION_UART         UART1
#define APP_TJC_UART            UART2
#define APP_MOTOR_UART          UART3

// SeekFree UART indexes and pin mux names.
#define APP_DEBUG_UART_INDEX    UART_0
#define APP_VISION_UART_INDEX   UART_1
#define APP_TJC_UART_INDEX      UART_2
#define APP_MOTOR_UART_INDEX    UART_3

#define APP_DEBUG_UART_TX_PIN   UART0_TX_A10
#define APP_DEBUG_UART_RX_PIN   UART0_RX_A11
#define APP_VISION_UART_TX_PIN  UART1_TX_A8
#define APP_VISION_UART_RX_PIN  UART1_RX_A9
#define APP_TJC_UART_TX_PIN     UART2_TX_B15
#define APP_TJC_UART_RX_PIN     UART2_RX_B16
#define APP_MOTOR_UART_TX_PIN   UART3_TX_B12
#define APP_MOTOR_UART_RX_PIN   UART3_RX_B13

#define APP_DEBUG_UART_BAUDRATE 115200
#define APP_VISION_UART_BAUDRATE 115200
#define APP_TJC_UART_BAUDRATE   115200
#define APP_MOTOR_UART_BAUDRATE 115200

// Board and external-interface GPIO allocation.
// SeekFree GPIO enums omit the physical-port prefix: B22 == PB22, etc.
#define LED_PIN                 B22
#define KEY_PIN                 B21
#define RS485_DE_PIN            B14

#define RS485_DE_TX_LEVEL       1
#define RS485_DE_RX_LEVEL       0

// Expansion-board frozen allocation.
// Board labels are written without the MCU port prefix: A8 == PA8, B12 == PB12.
// Keep these names in application code instead of scattering raw board pins.
#define VISION_UART_BOARD_TX_LABEL      "A8"
#define VISION_UART_BOARD_RX_LABEL      "A9"
#define TJC_UART_BOARD_TX_LABEL         "B15"
#define TJC_UART_BOARD_RX_LABEL         "B16"
#define X42S_UART_BOARD_TX_LABEL        "B12"
#define X42S_UART_BOARD_RX_LABEL        "B13"
#define RS485_DE_BOARD_LABEL            "B14"

// Optional 0.91 inch SSD1306 OLED, software IIC.
#define APP_OLED_SOFT_IIC_SCL_PIN       B4
#define APP_OLED_SOFT_IIC_SDA_PIN       B5

// Backup car chassis: AT8236 dual DC motor driver.
// These pins follow the teammate-verified car project.
#define CAR_MOTOR_A_IN1_PWM_PIN         PWM_TIM_G0_CH0_A12
#define CAR_MOTOR_A_IN2_PWM_PIN         PWM_TIM_G0_CH1_A13
#define CAR_MOTOR_B_IN1_PWM_PIN         PWM_TIM_G7_CH0_A26
#define CAR_MOTOR_B_IN2_PWM_PIN         PWM_TIM_G7_CH1_A27
#define CAR_MOTOR_PWM_FREQ_HZ           1000U
#define CAR_MOTOR_PWM_MAX               1000U
#define CAR_MOTOR_DEFAULT_TARGET_A_MMPS (-200)
#define CAR_MOTOR_DEFAULT_TARGET_B_MMPS 200
// Disabled in the yaw-only gimbal vision test build to stay under the
// non-commercial Keil 32 KB limit. Re-enable for car-only bring-up builds.
#define CAR_MOTION_ENABLED              0U
#define CAR_DEMO_AUTORUN_ENABLED        0U

// Backup car wheel encoders from the teammate-verified project.
#define CAR_ENCODER_A1_PIN              A14
#define CAR_ENCODER_B1_PIN              A15
#define CAR_ENCODER_A2_PIN              A24
#define CAR_ENCODER_B2_PIN              A25
#define CAR_ENCODER_PULSES_PER_REV      890U
#define CAR_ENCODER_WHEEL_DIAMETER_MM   67U
#define CAR_ENCODER_WHEEL_CIRCUM_X1000  210486U
#define CAR_ENCODER_SAMPLE_MS           50U

// Backup car line-following: RYDZ seven-channel grayscale sensor.
// The module outputs seven digital signals. This follows the verified project.
#define CAR_GRAY_SENSOR_COUNT           7U
#define CAR_GRAY_OUT1_PIN               B25
#define CAR_GRAY_OUT2_PIN               B24
#define CAR_GRAY_OUT3_PIN               B20
#define CAR_GRAY_OUT4_PIN               B18
#define CAR_GRAY_OUT5_PIN               B19
#define CAR_GRAY_OUT6_PIN               B10
#define CAR_GRAY_OUT7_PIN               A7

// Analog/current-sense reserve pins for later expansion.
#define APP_CURRENT_SENSE0_ADC_PIN      ADC1_CH4_B17
#define APP_CURRENT_SENSE1_ADC_PIN      APP_PIN_UNASSIGNED

// Spare PWM pair reserved for future servos or auxiliary mechanisms.
#define APP_AUX_PWM0_PIN                PWM_TIM_G6_CH0_B2
#define APP_AUX_PWM1_PIN                PWM_TIM_G6_CH1_B3

// TJC 7-inch serial HMI screen.
#define TJC_SCREEN_ENABLED              0U
#define TJC_SCREEN_MODEL                "TJC8048X270_011N"
#define TJC_SCREEN_WIDTH                800U
#define TJC_SCREEN_HEIGHT               480U

// MaixCAM binary protocol timing.
#define MAIXCAM_FRAME_HEADER_0          0xAAU
#define MAIXCAM_FRAME_HEADER_1          0x55U
#define MAIXCAM_REFRESH_PERIOD_MS       50U
#define MAIXCAM_TIMEOUT_MS              150U
#define MAIXCAM_MAX_PAYLOAD_LEN         16U
#define APP_UART_POLL_BUDGET             32U

// UART0 diagnostic log switch. Logs are state-transition oriented to avoid
// consuming the control loop with per-byte output.
#define APP_DEBUG_LOG_ENABLED            1U
#define APP_BOARD_SELF_TEST_ENABLED      1U
#define APP_BOARD_LED_HEARTBEAT_MS       500U
#define APP_VISION_RX_DEBUG_ENABLED      0U
#define APP_VISION_TX_SELF_TEST_ENABLED  1U
#define APP_VISION_TX_SELF_TEST_MS       1000U
#define APP_MOTOR_RX_DEBUG_ENABLED       0U
#define APP_GIMBAL_DEBUG_ENABLED         1U
#define APP_GIMBAL_DEBUG_PERIOD_MS       200U
#define APP_X42S_KEY_TEST_ENABLED        1U
#define APP_GIMBAL_RS485_TEST_BOOT_ENABLED 1U

// X42S motor configuration.
// Motor IDs, directions, zero points, and mechanical limits for the custom
// 3D-printed gimbal are not hardware-calibrated yet. The values below are
// commissioning defaults only; record confirmed values in
// docs/gimbal_parameter_confirmation.md before increasing motion ranges.
#define X42S_FIRMWARE_EMM_FREE          1U
#define VISION_ANGLE_UNITS_PER_DEG      100U
// Position commands use 3200 motion clocks per revolution. The Emm `0x36`
// real-time position reply instead reports a 16-bit encoder angle.
#define X42S_EMM_POSITION_PULSES_PER_REV 3200U
#define X42S_EMM_ENCODER_COUNTS_PER_REV  65536U
#define X42S_PITCH_MOTOR_ID             1U
#define X42S_YAW_MOTOR_ID               2U
#define X42S_DEFAULT_ACC_RPM_S_CONFIG   50U
#define X42S_DEFAULT_DEC_RPM_S_CONFIG   50U
#define X42S_DEFAULT_SPEED_0P1_RPM_CFG  200U
#define X42S_TEST_INTERFRAME_MS          40U

// Gimbal control parameters. Angles are stored in 0.01 degree unless noted.
// Mechanical bring-up on 2026-07-23 confirmed that a positive motor command
// moves ID1/Pitch downward and ID2/Yaw left. Automatic vision motion remains
// disabled until zero points and final soft limits are recorded.
#ifndef GIMBAL_MOTION_ENABLED
#define GIMBAL_MOTION_ENABLED            0U
#endif
// First closed-loop vision test: lock both X42S motors, but move only ID2/Yaw.
// Keep GIMBAL_MOTION_ENABLED disabled until full two-axis calibration is done.
#define GIMBAL_YAW_ONLY_TEST_ENABLED     1U
#define GIMBAL_CONTROL_PERIOD_MS        20U
#define GIMBAL_TARGET_LOST_TIMEOUT_MS   MAIXCAM_TIMEOUT_MS
#define GIMBAL_YAW_KP_NUM               1
#define GIMBAL_YAW_KP_DEN               10
#define GIMBAL_PITCH_KP_NUM             1
#define GIMBAL_PITCH_KP_DEN             10
#define GIMBAL_PITCH_POSITIVE_IS_DOWN   1U
#define GIMBAL_YAW_POSITIVE_IS_LEFT     1U
#define GIMBAL_PITCH_VISION_TO_MOTOR_SIGN (-1)
#define GIMBAL_YAW_VISION_TO_MOTOR_SIGN   (-1)
#define GIMBAL_YAW_MIN_0P1DEG           (-200)
#define GIMBAL_YAW_MAX_0P1DEG           200
#define GIMBAL_PITCH_MIN_0P1DEG         (-200)
#define GIMBAL_PITCH_MAX_0P1DEG         200
#define GIMBAL_STEP_LIMIT_0P1DEG        20
#define GIMBAL_YAW_ONLY_PERIOD_MS       80U
#define GIMBAL_YAW_ONLY_MIN_0P1DEG      (-300)
#define GIMBAL_YAW_ONLY_MAX_0P1DEG      300
#define GIMBAL_YAW_ONLY_STEP_LIMIT_0P1DEG 60
#define GIMBAL_YAW_ONLY_DEADBAND_0P01DEG 300
#define GIMBAL_YAW_ONLY_START_DEADBAND_0P01DEG 500
#define GIMBAL_YAW_ONLY_TARGET_HYST_0P1DEG 12
#define GIMBAL_YAW_ONLY_CONF_MIN_0P01PCT 6500U
#define GIMBAL_YAW_ONLY_CONF_START_MIN_0P01PCT 7000U
#define GIMBAL_YAW_ONLY_VALID_FRAMES_MIN 3U
#define GIMBAL_YAW_ONLY_ANGLE_GAIN_NUM  13
#define GIMBAL_YAW_ONLY_ANGLE_GAIN_DEN  10
#define GIMBAL_YAW_ONLY_FILTER_SHIFT    1U
#define GIMBAL_YAW_ONLY_ACC_RPM_S       60U
#define GIMBAL_YAW_ONLY_SPEED_0P1_RPM   500U
#define GIMBAL_YAW_ONLY_SPEED_MODE      1U
#define GIMBAL_YAW_ONLY_SPEED_PERIOD_MS 60U
#define GIMBAL_YAW_ONLY_SPEED_MAX_0P1_RPM 120
#define GIMBAL_YAW_ONLY_SPEED_MIN_0P1_RPM 10
#define GIMBAL_YAW_ONLY_SPEED_KP_NUM    1
#define GIMBAL_YAW_ONLY_SPEED_KP_DEN    12
#define GIMBAL_YAW_ONLY_SPEED_KD_NUM    1
#define GIMBAL_YAW_ONLY_SPEED_KD_DEN    10
#define GIMBAL_YAW_ONLY_SPEED_HYST_0P1_RPM 8
#define GIMBAL_YAW_ONLY_SPEED_STEP_0P1_RPM 30
#define GIMBAL_YAW_ONLY_POS_REQ_MS      80U
#define GIMBAL_YAW_ONLY_POS_FEEDBACK_TIMEOUT_MS 250U
#define GIMBAL_YAW_ONLY_STOP_REFRESH_MS 50U

#endif
