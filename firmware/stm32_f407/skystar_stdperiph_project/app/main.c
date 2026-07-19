/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * 

 Change Logs:
 * Date           Author       Notes
 * 2024-03-07     LCKFB-LP    first version
 */
#include "board.h"
#include "bsp_uart.h"
#include "skystar_key.h"
#include "skystar_oled.h"
#include "skystar_x42s.h"
#include "vision_ascii_protocol.h"
#include <stdio.h>

static VisionAscii_Parser g_vision_parser;

static void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void print_0p1(int32_t value)
{
	if (value < 0) {
		printf("-");
		value = -value;
	}
	printf("%ld.%ld", (long)(value / 10), (long)(value % 10));
}

static void format_0p1(char *buffer, uint32_t size, int32_t value)
{
	if (value < 0) {
		snprintf(buffer, size, "-%ld.%ld", (long)(-value / 10), (long)(-value % 10));
	} else {
		snprintf(buffer, size, "%ld.%ld", (long)(value / 10), (long)(value % 10));
	}
}

static void oled_show_waiting(uint32_t key_count)
{
	SkystarOled_ShowLine(0U, "SKYSTAR F407");
	SkystarOled_ShowLine(1U, "WAIT VISION");
	SkystarOled_ShowLine(2U, "USART2 PA2 PA3");
	char line[32];
	snprintf(line, sizeof(line), "KEY %lu 115200", (unsigned long)key_count);
	SkystarOled_ShowLine(3U, line);
}

static void oled_show_result(const VisionAscii_Result *result, uint8_t connected, uint32_t key_count)
{
	char line[32];
	char angle[16];
	char distance[16];

	format_0p1(angle, sizeof(angle), result->angle_0p1deg);
	format_0p1(distance, sizeof(distance), result->distance_0p1cm);

	snprintf(line, sizeof(line), "VISION %s", connected ? "OK" : "LOST");
	SkystarOled_ShowLine(0U, line);

	snprintf(line, sizeof(line), "CX %d CY %d", (int)result->cx, (int)result->cy);
	SkystarOled_ShowLine(1U, line);

	snprintf(line, sizeof(line), "A %s D %s", angle, distance);
	SkystarOled_ShowLine(2U, line);

	snprintf(line, sizeof(line), "SEQ %u KEY %lu", (unsigned)result->sequence, (unsigned long)key_count);
	SkystarOled_ShowLine(3U, line);
}

static void oled_show_timeout(uint32_t key_count)
{
	char line[32];

	SkystarOled_ShowLine(0U, "VISION TIMEOUT");
	SkystarOled_ShowLine(1U, "NO VALID TARGET");
	SkystarOled_ShowLine(2U, "CHECK MAIXCAM");
	snprintf(line, sizeof(line), "KEY %lu", (unsigned long)key_count);
	SkystarOled_ShowLine(3U, line);
}

void uart2_rx_callback(uint8_t data)
{
	(void)VisionAscii_InputByte(&g_vision_parser, data);
}

void uart3_rx_callback(uint8_t data)
{
	SkystarX42S_OnRxByte(data);
}

int main(void)
{
	VisionAscii_Result result;
	uint32_t now_ms = 0U;
	uint32_t last_frame_ms = 0U;
	uint8_t connected = 0U;
	uint8_t last_connected = 0U;
	uint32_t key_count = 0U;

	board_init();
	uart1_init(115200U);
	uart2_init(115200U);
	led_init();
	SkystarKey_Init();
	SkystarOled_Init();
	SkystarX42S_Init();
	VisionAscii_Init(&g_vision_parser);

	printf("\r\nSKYSTAR F407 VISION UART DEMO\r\n");
	printf("DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, X42S: USART3 PB10/PB11, KEY: PA0, OLED: PB8/PB9, 115200 8N1\r\n");
	oled_show_waiting(key_count);
	
	while(1)
	{
		uint8_t got_frame;

		__disable_irq();
		got_frame = VisionAscii_TakeResult(&g_vision_parser, &result);
		__enable_irq();

		if (SkystarKey_Update(now_ms) != 0U) {
			key_count++;
			printf("KEY PRESS count=%lu\r\n", (unsigned long)key_count);
			SkystarX42S_ReadPosition(SKYSTAR_X42S_DEFAULT_ID);
			printf("X42S READ POSITION id=%u last=%ld(0.1deg)\r\n",
			       (unsigned)SKYSTAR_X42S_DEFAULT_ID,
			       (long)SkystarX42S_GetLastPosition(SKYSTAR_X42S_DEFAULT_ID));
			if (got_frame) {
				/* The fresh frame below will refresh the OLED. */
			} else if (connected) {
				SkystarOled_ShowLine(3U, "KEY PRESSED");
			} else {
				oled_show_timeout(key_count);
			}
		}

		if (got_frame) {
			last_frame_ms = now_ms;
			connected = result.valid ? 1U : 0U;
			printf("VISION %s seq=%u mode=%u cx=%d cy=%d w=%u h=%u dist=",
			       connected ? "OK" : "LOST",
			       (unsigned)result.sequence,
			       (unsigned)result.mode,
			       (int)result.cx,
			       (int)result.cy,
			       (unsigned)result.width,
			       (unsigned)result.height);
			print_0p1(result.distance_0p1cm);
			printf("cm size=");
			print_0p1(result.size_0p1cm);
			printf("cm angle=");
			print_0p1(result.angle_0p1deg);
			printf("deg\r\n");
			oled_show_result(&result, connected, key_count);
		}

		if (connected && (now_ms - last_frame_ms > VISION_ASCII_TIMEOUT_MS)) {
			connected = 0U;
		}

		if (connected != last_connected) {
			printf("VISION LINK %s\r\n", connected ? "CONNECTED" : "TIMEOUT");
			if (!connected) {
				oled_show_timeout(key_count);
			}
			last_connected = connected;
		}

		if (connected) {
			GPIO_SetBits(GPIOB, GPIO_Pin_2);
		} else {
			GPIO_ResetBits(GPIOB, GPIO_Pin_2);
		}

		delay_ms(100);
		now_ms += 100U;
	}
}

