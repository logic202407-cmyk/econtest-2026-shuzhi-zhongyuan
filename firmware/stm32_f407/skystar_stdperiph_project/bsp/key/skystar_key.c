#include "skystar_key.h"
#include "stm32f4xx.h"

#define SKYSTAR_KEY_GPIO       GPIOA
#define SKYSTAR_KEY_PIN        GPIO_Pin_0
#define SKYSTAR_KEY_DEBOUNCE_MS 30U

static uint8_t g_stable_pressed;
static uint8_t g_last_sample;
static uint32_t g_last_change_ms;

void SkystarKey_Init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = SKYSTAR_KEY_PIN;
	gpio.GPIO_Mode = GPIO_Mode_IN;
	gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(SKYSTAR_KEY_GPIO, &gpio);

	g_stable_pressed = 0U;
	g_last_sample = 0U;
	g_last_change_ms = 0U;
}

uint8_t SkystarKey_IsPressed(void)
{
	return (GPIO_ReadInputDataBit(SKYSTAR_KEY_GPIO, SKYSTAR_KEY_PIN) == SET) ? 1U : 0U;
}

uint8_t SkystarKey_Update(uint32_t now_ms)
{
	uint8_t sample = SkystarKey_IsPressed();

	if (sample != g_last_sample) {
		g_last_sample = sample;
		g_last_change_ms = now_ms;
	}

	if ((sample != g_stable_pressed) && ((now_ms - g_last_change_ms) >= SKYSTAR_KEY_DEBOUNCE_MS)) {
		g_stable_pressed = sample;
		if (g_stable_pressed != 0U) {
			return 1U;
		}
	}

	return 0U;
}
