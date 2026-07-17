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
    PLATFORM_UART_MOTOR
} Platform_Uart;

void Platform_UartInit(Platform_Uart uart);
bool Platform_UartReadByte(Platform_Uart uart, uint8_t *byte);
void Platform_UartWrite(Platform_Uart uart, const uint8_t *data, size_t len);
void Platform_UartWaitTxComplete(Platform_Uart uart);
void Platform_Rs485Write(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
