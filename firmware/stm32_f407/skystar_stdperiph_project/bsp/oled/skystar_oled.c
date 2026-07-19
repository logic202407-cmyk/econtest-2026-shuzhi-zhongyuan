#include "skystar_oled.h"
#include "stm32f4xx.h"

#define OLED_I2C_ADDR_WRITE 0x78U
#define OLED_SCL_PIN        GPIO_Pin_8
#define OLED_SDA_PIN        GPIO_Pin_9
#define OLED_GPIO           GPIOB

typedef struct {
	char ch;
	uint8_t data[5];
} OledGlyph;

static const OledGlyph k_font[] = {
	{' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
	{'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
	{'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
	{':', {0x00, 0x36, 0x36, 0x00, 0x00}},
	{'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
	{'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
	{'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
	{'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
	{'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
	{'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
	{'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
	{'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
	{'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
	{'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
	{'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
	{'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
	{'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
	{'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
	{'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
	{'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
	{'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
	{'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
	{'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
	{'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
	{'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
	{'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
	{'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
	{'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
	{'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
	{'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
	{'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
	{'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
	{'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
	{'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
	{'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
	{'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
	{'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
	{'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
	{'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
	{'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
	{'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
};

static void oled_delay(void)
{
	volatile uint32_t i;
	for (i = 0; i < 16U; i++) {
		__NOP();
	}
}

static void oled_scl(uint8_t level)
{
	if (level != 0U) {
		GPIO_SetBits(OLED_GPIO, OLED_SCL_PIN);
	} else {
		GPIO_ResetBits(OLED_GPIO, OLED_SCL_PIN);
	}
	oled_delay();
}

static void oled_sda(uint8_t level)
{
	if (level != 0U) {
		GPIO_SetBits(OLED_GPIO, OLED_SDA_PIN);
	} else {
		GPIO_ResetBits(OLED_GPIO, OLED_SDA_PIN);
	}
	oled_delay();
}

static void oled_i2c_start(void)
{
	oled_sda(1U);
	oled_scl(1U);
	oled_sda(0U);
	oled_scl(0U);
}

static void oled_i2c_stop(void)
{
	oled_sda(0U);
	oled_scl(1U);
	oled_sda(1U);
}

static void oled_i2c_write(uint8_t value)
{
	uint8_t i;

	for (i = 0U; i < 8U; i++) {
		oled_sda((value & 0x80U) ? 1U : 0U);
		oled_scl(1U);
		oled_scl(0U);
		value <<= 1U;
	}

	oled_sda(1U);
	oled_scl(1U);
	oled_scl(0U);
}

static void oled_write_command(uint8_t command)
{
	oled_i2c_start();
	oled_i2c_write(OLED_I2C_ADDR_WRITE);
	oled_i2c_write(0x00U);
	oled_i2c_write(command);
	oled_i2c_stop();
}

static void oled_write_data(uint8_t data)
{
	oled_i2c_start();
	oled_i2c_write(OLED_I2C_ADDR_WRITE);
	oled_i2c_write(0x40U);
	oled_i2c_write(data);
	oled_i2c_stop();
}

static void oled_set_cursor(uint8_t page, uint8_t column)
{
	oled_write_command((uint8_t)(0xB0U | (page & 0x03U)));
	oled_write_command((uint8_t)(0x10U | ((column >> 4U) & 0x0FU)));
	oled_write_command((uint8_t)(column & 0x0FU));
}

static const uint8_t *oled_find_glyph(char ch)
{
	uint32_t i;

	if ((ch >= 'a') && (ch <= 'z')) {
		ch = (char)(ch - 'a' + 'A');
	}

	for (i = 0U; i < (sizeof(k_font) / sizeof(k_font[0])); i++) {
		if (k_font[i].ch == ch) {
			return k_font[i].data;
		}
	}

	return k_font[0].data;
}

static void oled_show_char(char ch)
{
	const uint8_t *glyph = oled_find_glyph(ch);
	uint8_t i;

	for (i = 0U; i < 5U; i++) {
		oled_write_data(glyph[i]);
	}
	oled_write_data(0x00U);
}

void SkystarOled_Init(void)
{
	GPIO_InitTypeDef gpio;
	volatile uint32_t i;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = OLED_SCL_PIN | OLED_SDA_PIN;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(OLED_GPIO, &gpio);

	oled_scl(1U);
	oled_sda(1U);

	for (i = 0U; i < 120000U; i++) {
		__NOP();
	}

	oled_write_command(0xAEU);
	oled_write_command(0xD5U);
	oled_write_command(0x80U);
	oled_write_command(0xA8U);
	oled_write_command(0x1FU);
	oled_write_command(0xD3U);
	oled_write_command(0x00U);
	oled_write_command(0x40U);
	oled_write_command(0xA1U);
	oled_write_command(0xC8U);
	oled_write_command(0xDAU);
	oled_write_command(0x02U);
	oled_write_command(0x81U);
	oled_write_command(0xCFU);
	oled_write_command(0xD9U);
	oled_write_command(0xF1U);
	oled_write_command(0xDBU);
	oled_write_command(0x30U);
	oled_write_command(0xA4U);
	oled_write_command(0xA6U);
	oled_write_command(0x8DU);
	oled_write_command(0x14U);
	oled_write_command(0xAFU);

	SkystarOled_Clear();
}

void SkystarOled_Clear(void)
{
	uint8_t page;
	uint8_t column;

	for (page = 0U; page < SKYSTAR_OLED_ROWS; page++) {
		oled_set_cursor(page, 0U);
		for (column = 0U; column < 128U; column++) {
			oled_write_data(0x00U);
		}
	}
}

void SkystarOled_ShowLine(uint8_t row, const char *text)
{
	uint8_t column;

	if ((row >= SKYSTAR_OLED_ROWS) || (text == 0)) {
		return;
	}

	oled_set_cursor(row, 0U);
	for (column = 0U; column < SKYSTAR_OLED_COLUMNS; column++) {
		if (text[column] != '\0') {
			oled_show_char(text[column]);
		} else {
			oled_show_char(' ');
		}
	}

	while ((uint16_t)column * 6U < 128U) {
		oled_write_data(0x00U);
		column++;
	}
}
