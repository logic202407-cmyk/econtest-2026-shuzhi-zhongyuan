#ifndef PLATFORM_UART_H
#define PLATFORM_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PLATFORM_UART_DEBUG = 0,
    PLATFORM_UART_VISION,
    PLATFORM_UART_TJC_SCREEN,
    PLATFORM_UART_MOTOR
} Platform_Uart;

void platform_uart_init(Platform_Uart uart);
bool platform_uart_receive(Platform_Uart uart, uint8_t *byte);
void platform_uart_send(Platform_Uart uart, const uint8_t *data, size_t len);
void platform_uart_wait_tx_complete(Platform_Uart uart);
void platform_rs485_send(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
