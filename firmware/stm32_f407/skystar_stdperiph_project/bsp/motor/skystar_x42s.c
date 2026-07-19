#include "skystar_x42s.h"
#include "bsp_uart.h"
#include "stm32f4xx.h"
#include <stddef.h>

#define X42S_CHECK_FIXED       0x6BU
#define X42S_MAX_ID            32U
#define X42S_DEFAULT_ACC_RPM_S 100U
#define X42S_DEFAULT_DEC_RPM_S 100U
#define X42S_DEFAULT_SPEED_0P1_RPM 300U

#define X42S_DE_GPIO GPIOB
#define X42S_DE_PIN  GPIO_Pin_12

static int32_t g_last_position[X42S_MAX_ID + 1U];
static uint8_t g_rx_buf[16];
static uint8_t g_rx_len;

static uint32_t abs_i32(int32_t value)
{
	if (value < 0) {
		return (uint32_t)(-(value + 1)) + 1U;
	}

	return (uint32_t)value;
}

static void put_u16(uint8_t *buf, uint16_t value)
{
	buf[0] = (uint8_t)(value >> 8);
	buf[1] = (uint8_t)value;
}

static void put_u32(uint8_t *buf, uint32_t value)
{
	buf[0] = (uint8_t)(value >> 24);
	buf[1] = (uint8_t)(value >> 16);
	buf[2] = (uint8_t)(value >> 8);
	buf[3] = (uint8_t)value;
}

static uint32_t get_u32(const uint8_t *buf)
{
	return ((uint32_t)buf[0] << 24) |
	       ((uint32_t)buf[1] << 16) |
	       ((uint32_t)buf[2] << 8) |
	       (uint32_t)buf[3];
}

static uint8_t direction_from_signed(int32_t value)
{
	return (value < 0) ? 1U : 0U;
}

static void set_rs485_tx(uint8_t enable)
{
	if (enable != 0U) {
		GPIO_SetBits(X42S_DE_GPIO, X42S_DE_PIN);
	} else {
		GPIO_ResetBits(X42S_DE_GPIO, X42S_DE_PIN);
	}
}

static void send_frame(const uint8_t *frame, size_t len)
{
	set_rs485_tx(1U);
	uart3_send_bytes(frame, len);
	set_rs485_tx(0U);
}

void SkystarX42S_Init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = X42S_DE_PIN;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(X42S_DE_GPIO, &gpio);

	set_rs485_tx(0U);
	uart3_init(115200U);
}

void SkystarX42S_Enable(uint8_t id)
{
	uint8_t frame[] = {id, 0xF3U, 0xABU, 0x01U, 0x00U, X42S_CHECK_FIXED};
	send_frame(frame, sizeof(frame));
}

void SkystarX42S_Disable(uint8_t id)
{
	uint8_t frame[] = {id, 0xF3U, 0xABU, 0x00U, 0x00U, X42S_CHECK_FIXED};
	send_frame(frame, sizeof(frame));
}

void SkystarX42S_Stop(uint8_t id)
{
	uint8_t frame[] = {id, 0xFEU, 0x98U, 0x00U, X42S_CHECK_FIXED};
	send_frame(frame, sizeof(frame));
}

void SkystarX42S_ReadPosition(uint8_t id)
{
	uint8_t frame[] = {id, 0x0FU, X42S_CHECK_FIXED};
	send_frame(frame, sizeof(frame));
}

void SkystarX42S_SetPosition(uint8_t id, int32_t position_0p1deg)
{
	uint8_t frame[16] = {0};

	frame[0] = id;
	frame[1] = 0xFDU;
	frame[2] = direction_from_signed(position_0p1deg);
	put_u16(&frame[3], X42S_DEFAULT_ACC_RPM_S);
	put_u16(&frame[5], X42S_DEFAULT_DEC_RPM_S);
	put_u16(&frame[7], X42S_DEFAULT_SPEED_0P1_RPM);
	put_u32(&frame[9], abs_i32(position_0p1deg));
	frame[13] = 2U;
	frame[14] = 0U;
	frame[15] = X42S_CHECK_FIXED;

	send_frame(frame, sizeof(frame));
}

void SkystarX42S_SetSpeed(uint8_t id, int32_t speed_0p1rpm)
{
	uint8_t frame[9] = {0};

	frame[0] = id;
	frame[1] = 0xF6U;
	frame[2] = direction_from_signed(speed_0p1rpm);
	put_u16(&frame[3], X42S_DEFAULT_ACC_RPM_S);
	put_u16(&frame[5], (uint16_t)abs_i32(speed_0p1rpm));
	frame[7] = 0U;
	frame[8] = X42S_CHECK_FIXED;

	send_frame(frame, sizeof(frame));
}

void SkystarX42S_OnRxByte(uint8_t byte)
{
	if (g_rx_len < sizeof(g_rx_buf)) {
		g_rx_buf[g_rx_len++] = byte;
	} else {
		g_rx_len = 0U;
	}

	if (byte != X42S_CHECK_FIXED) {
		return;
	}

	if (g_rx_len >= 8U) {
		uint8_t id = g_rx_buf[0];
		uint8_t cmd = g_rx_buf[1];
		uint8_t sign = g_rx_buf[2];

		if ((id <= X42S_MAX_ID) && (cmd == 0x0FU)) {
			int32_t position = (int32_t)get_u32(&g_rx_buf[3]);
			g_last_position[id] = (sign != 0U) ? -position : position;
		}
	}

	g_rx_len = 0U;
}

int32_t SkystarX42S_GetLastPosition(uint8_t id)
{
	if (id > X42S_MAX_ID) {
		return 0;
	}

	return g_last_position[id];
}
