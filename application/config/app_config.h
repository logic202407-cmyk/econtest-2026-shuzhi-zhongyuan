#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Central hardware allocation for the TianMengXing MSPM0G3507 application.
// Application code should include this file instead of spelling raw pins.

// UART peripherals requested by the application architecture.
#define DEBUG_UART              UART0
#define VISION_UART             UART1
#define MOTOR_UART              UART2

// SeekFree UART indexes and pin mux names.
#define DEBUG_UART_INDEX        UART_0
#define VISION_UART_INDEX       UART_1
#define MOTOR_UART_INDEX        UART_2

#define DEBUG_UART_TX_PIN       UART0_TX_A10
#define DEBUG_UART_RX_PIN       UART0_RX_A11
#define VISION_UART_TX_PIN      UART1_TX_A8
#define VISION_UART_RX_PIN      UART1_RX_A9
#define MOTOR_UART_TX_PIN       UART2_TX_B15
#define MOTOR_UART_RX_PIN       UART2_RX_B16

#define DEBUG_UART_BAUDRATE     115200
#define VISION_UART_BAUDRATE    115200
#define MOTOR_UART_BAUDRATE     115200

// Board and external-interface GPIO allocation.
#define LED_PIN                 PB22
#define KEY_PIN                 PB21
#define RS485_DE_PIN            PB17

#define RS485_DE_TX_LEVEL       1
#define RS485_DE_RX_LEVEL       0

// MaixCAM binary protocol timing.
#define MAIXCAM_FRAME_HEADER_0          0xAAU
#define MAIXCAM_FRAME_HEADER_1          0x55U
#define MAIXCAM_REFRESH_PERIOD_MS       50U
#define MAIXCAM_TIMEOUT_MS              500U
#define MAIXCAM_MAX_PAYLOAD_LEN         16U
#define APP_UART_POLL_BUDGET             32U

// X42S motor configuration.
#define X42S_YAW_MOTOR_ID               1U
#define X42S_PITCH_MOTOR_ID             2U
#define X42S_DEFAULT_ACC_RPM_S_CONFIG   100U
#define X42S_DEFAULT_DEC_RPM_S_CONFIG   100U
#define X42S_DEFAULT_SPEED_0P1_RPM_CFG  300U

// Gimbal control parameters. Angles are stored in 0.01 degree unless noted.
#define GIMBAL_CONTROL_PERIOD_MS        20U
#define GIMBAL_TARGET_LOST_TIMEOUT_MS   MAIXCAM_TIMEOUT_MS
#define GIMBAL_YAW_KP_NUM               1
#define GIMBAL_YAW_KP_DEN               10
#define GIMBAL_PITCH_KP_NUM             1
#define GIMBAL_PITCH_KP_DEN             10
#define GIMBAL_YAW_MIN_0P1DEG           (-900)
#define GIMBAL_YAW_MAX_0P1DEG           900
#define GIMBAL_PITCH_MIN_0P1DEG         (-300)
#define GIMBAL_PITCH_MAX_0P1DEG         300
#define GIMBAL_STEP_LIMIT_0P1DEG        20

#endif
