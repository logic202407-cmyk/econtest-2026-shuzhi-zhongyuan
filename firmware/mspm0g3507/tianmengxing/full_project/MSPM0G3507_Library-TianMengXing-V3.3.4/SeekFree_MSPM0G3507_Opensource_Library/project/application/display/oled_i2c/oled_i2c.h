#ifndef OLED_I2C_H
#define OLED_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_I2C_WIDTH     128U
#define OLED_I2C_HEIGHT    64U
#define OLED_I2C_PAGES     8U

void OledI2c_Init(void);
void OledI2c_Clear(void);
void OledI2c_ShowString(uint8_t x, uint8_t page, const char *text);
void OledI2c_ShowLargeString(uint8_t x, uint8_t page, const char *text,
                             uint8_t scale);
void OledI2c_ShowInt(uint8_t x, uint8_t page, int32_t value, uint8_t width);

#ifdef __cplusplus
}
#endif

#endif
