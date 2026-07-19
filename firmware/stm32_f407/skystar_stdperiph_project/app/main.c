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

void uart2_rx_callback(uint8_t data)
{
	(void)VisionAscii_InputByte(&g_vision_parser, data);
}

int main(void)
{
	VisionAscii_Result result;
	uint32_t now_ms = 0U;
	uint32_t last_frame_ms = 0U;
	uint8_t connected = 0U;
	uint8_t last_connected = 0U;

	board_init();
	uart1_init(115200U);
	uart2_init(115200U);
	led_init();
	VisionAscii_Init(&g_vision_parser);

	printf("\r\nSKYSTAR F407 VISION UART DEMO\r\n");
	printf("DEBUG: USART1 PA9/PA10, MaixCAM: USART2 PA2/PA3, 115200 8N1\r\n");
	
	while(1)
	{
		uint8_t got_frame;

		__disable_irq();
		got_frame = VisionAscii_TakeResult(&g_vision_parser, &result);
		__enable_irq();

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
		}

		if (connected && (now_ms - last_frame_ms > VISION_ASCII_TIMEOUT_MS)) {
			connected = 0U;
		}

		if (connected != last_connected) {
			printf("VISION LINK %s\r\n", connected ? "CONNECTED" : "TIMEOUT");
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

