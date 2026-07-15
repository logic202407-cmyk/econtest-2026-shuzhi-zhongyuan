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

#endif
