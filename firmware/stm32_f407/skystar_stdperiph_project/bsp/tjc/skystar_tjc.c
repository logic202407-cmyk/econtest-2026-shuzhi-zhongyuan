#include "skystar_tjc.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <string.h>

static void tjc_send_tail(void)
{
	static const uint8_t tail[3] = {0xFFU, 0xFFU, 0xFFU};
	uart6_send_bytes(tail, sizeof(tail));
}

void SkystarTjc_Init(void)
{
#if SKYSTAR_TJC_ENABLED
	uart6_init(SKYSTAR_TJC_BAUDRATE);
	SkystarTjc_SendCommand("bkcmd=3");
#endif
}

void SkystarTjc_SendCommand(const char *command)
{
	if (command == NULL) {
		return;
	}

	uart6_send_bytes((const uint8_t *)command, (uint32_t)strlen(command));
	tjc_send_tail();
}

void SkystarTjc_SetBrightness(uint8_t percent)
{
	char command[16];

	if (percent > 100U) {
		percent = 100U;
	}

	snprintf(command, sizeof(command), "dim=%u", (unsigned)percent);
	SkystarTjc_SendCommand(command);
}

void SkystarTjc_SetText(const char *object_name, const char *text)
{
	char command[96];

	if ((object_name == NULL) || (text == NULL)) {
		return;
	}

	snprintf(command, sizeof(command), "%s.txt=\"%s\"", object_name, text);
	SkystarTjc_SendCommand(command);
}

void SkystarTjc_SetNumber(const char *object_name, int32_t value)
{
	char command[48];

	if (object_name == NULL) {
		return;
	}

	snprintf(command, sizeof(command), "%s.val=%ld", object_name, (long)value);
	SkystarTjc_SendCommand(command);
}

void SkystarTjc_ShowBasicStatus(const char *mode,
                                int32_t yaw_0p1deg,
                                int32_t pitch_0p1deg,
                                uint8_t vision_ok)
{
	SkystarTjc_SetText("t_mode", mode == NULL ? "IDLE" : mode);
	SkystarTjc_SetText("t_vis", vision_ok ? "VISION OK" : "VISION LOST");
	SkystarTjc_SetNumber("n_yaw", yaw_0p1deg);
	SkystarTjc_SetNumber("n_pitch", pitch_0p1deg);
}
