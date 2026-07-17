#include "platform_uart.h"

#include "zf_common_headfile.h"

#include "../config/app_config.h"
#include "platform_gpio.h"

static uart_index_enum platform_uart_index(Platform_Uart uart)
{
    switch (uart) {
    case PLATFORM_UART_DEBUG:
        return DEBUG_UART_INDEX;
    case PLATFORM_UART_VISION:
        return VISION_UART_INDEX;
    case PLATFORM_UART_MOTOR:
    default:
        return MOTOR_UART_INDEX;
    }
}

static UART_Regs *platform_uart_regs(Platform_Uart uart)
{
    switch (uart) {
    case PLATFORM_UART_DEBUG:
        return DEBUG_UART;
    case PLATFORM_UART_VISION:
        return VISION_UART;
    case PLATFORM_UART_MOTOR:
    default:
        return MOTOR_UART;
    }
}

void Platform_UartInit(Platform_Uart uart)
{
    switch (uart) {
    case PLATFORM_UART_DEBUG:
        uart_init(DEBUG_UART_INDEX, DEBUG_UART_BAUDRATE,
                  DEBUG_UART_TX_PIN, DEBUG_UART_RX_PIN);
        break;

    case PLATFORM_UART_VISION:
        uart_init(VISION_UART_INDEX, VISION_UART_BAUDRATE,
                  VISION_UART_TX_PIN, VISION_UART_RX_PIN);
        break;

    case PLATFORM_UART_MOTOR:
    default:
        uart_init(MOTOR_UART_INDEX, MOTOR_UART_BAUDRATE,
                  MOTOR_UART_TX_PIN, MOTOR_UART_RX_PIN);
        break;
    }
}

bool Platform_UartReadByte(Platform_Uart uart, uint8_t *byte)
{
    if (byte == NULL) {
        return false;
    }

    return uart_query_byte(platform_uart_index(uart), (uint8 *)byte) == ZF_TRUE;
}

void Platform_UartWrite(Platform_Uart uart, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0U) {
        return;
    }

    uart_write_buffer(platform_uart_index(uart), (const uint8 *)data,
                      (uint32)len);
    Platform_UartWaitTxComplete(uart);
}

void Platform_UartWaitTxComplete(Platform_Uart uart)
{
    while (DL_UART_isBusy(platform_uart_regs(uart))) {
    }
}

void Platform_Rs485Write(const uint8_t *data, size_t len)
{
    Platform_Rs485SetTxEnable(true);
    Platform_UartWrite(PLATFORM_UART_MOTOR, data, len);
    Platform_Rs485SetTxEnable(false);
}

void App_DebugUartInit(void)
{
    Platform_UartInit(PLATFORM_UART_DEBUG);
}

void App_VisionUartInit(void)
{
    Platform_UartInit(PLATFORM_UART_VISION);
}

void App_MotorUartInit(void)
{
    Platform_UartInit(PLATFORM_UART_MOTOR);
    Platform_Rs485SetTxEnable(false);
}

bool App_VisionReadByte(uint8_t *byte)
{
    return Platform_UartReadByte(PLATFORM_UART_VISION, byte);
}

bool App_MotorReadByte(uint8_t *byte)
{
    return Platform_UartReadByte(PLATFORM_UART_MOTOR, byte);
}

void App_MotorSend(const uint8_t *data, size_t len)
{
    Platform_UartWrite(PLATFORM_UART_MOTOR, data, len);
}
