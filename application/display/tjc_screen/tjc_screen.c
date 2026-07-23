#include "tjc_screen.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../../config/app_config.h"
#include "../../platform/platform_uart.h"

#define TJC_CMD_BUF_LEN 96U

static void send_terminator(void)
{
    static const uint8_t end_bytes[3] = {0xFFU, 0xFFU, 0xFFU};

    platform_uart_send(PLATFORM_UART_TJC_SCREEN, end_bytes, sizeof(end_bytes));
}

void TJC_ScreenInit(void)
{
#if TJC_SCREEN_ENABLED
    platform_uart_init(PLATFORM_UART_TJC_SCREEN);
    TJC_SendCommand("bkcmd=3");
#endif
}

void TJC_SendCommand(const char *command)
{
    if (command == NULL) {
        return;
    }

    platform_uart_send(PLATFORM_UART_TJC_SCREEN, (const uint8_t *)command,
                       strlen(command));
    send_terminator();
}

void TJC_SetBrightness(uint8_t percent)
{
    char command[TJC_CMD_BUF_LEN];

    if (percent > 100U) {
        percent = 100U;
    }

    (void)snprintf(command, sizeof(command), "dim=%u", (unsigned int)percent);
    TJC_SendCommand(command);
}

void TJC_SetText(const char *object_name, const char *text)
{
    char command[TJC_CMD_BUF_LEN];

    if (object_name == NULL || text == NULL) {
        return;
    }

    (void)snprintf(command, sizeof(command), "%s.txt=\"%s\"", object_name, text);
    TJC_SendCommand(command);
}

void TJC_SetNumber(const char *object_name, int32_t value)
{
    char command[TJC_CMD_BUF_LEN];

    if (object_name == NULL) {
        return;
    }

    (void)snprintf(command, sizeof(command), "%s.val=%ld", object_name,
                   (long)value);
    TJC_SendCommand(command);
}

void TJC_ShowBasicStatus(const char *mode, const char *vision, int32_t yaw_0p01deg,
                         int32_t pitch_0p01deg, const char *motor)
{
    char value[TJC_CMD_BUF_LEN];

    TJC_SetText("t_mode", mode != NULL ? mode : "-");
    TJC_SetText("t_vis", vision != NULL ? vision : "-");

    (void)snprintf(value, sizeof(value), "yaw=%ld.%02ld",
                   (long)(yaw_0p01deg / 100),
                   (long)((yaw_0p01deg < 0 ? -yaw_0p01deg : yaw_0p01deg) % 100));
    TJC_SetText("t_yaw", value);

    (void)snprintf(value, sizeof(value), "pitch=%ld.%02ld",
                   (long)(pitch_0p01deg / 100),
                   (long)((pitch_0p01deg < 0 ? -pitch_0p01deg : pitch_0p01deg) % 100));
    TJC_SetText("t_pitch", value);

    TJC_SetText("t_motor", motor != NULL ? motor : "-");
}
