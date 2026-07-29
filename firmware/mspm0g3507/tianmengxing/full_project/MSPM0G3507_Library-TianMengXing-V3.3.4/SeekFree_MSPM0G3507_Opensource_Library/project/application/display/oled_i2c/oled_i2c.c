#include "oled_i2c.h"

#include "zf_common_headfile.h"

#include "../../config/app_config.h"

#define OLED_I2C_ADDR          0x3CU
#define OLED_I2C_DELAY         1U
#define OLED_CONTROL_CMD       0x00U
#define OLED_CONTROL_DATA      0x40U

static soft_iic_info_struct g_oled_iic;

static const uint8_t *font5x7(char ch)
{
    static const uint8_t blank[5] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    static const uint8_t minus[5] = {0x08U, 0x08U, 0x08U, 0x08U, 0x08U};
    static const uint8_t slash[5] = {0x20U, 0x10U, 0x08U, 0x04U, 0x02U};
    static const uint8_t colon[5] = {0x00U, 0x36U, 0x36U, 0x00U, 0x00U};
    static const uint8_t zero[5] = {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU};
    static const uint8_t one[5] = {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U};
    static const uint8_t two[5] = {0x42U, 0x61U, 0x51U, 0x49U, 0x46U};
    static const uint8_t three[5] = {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U};
    static const uint8_t four[5] = {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U};
    static const uint8_t five[5] = {0x27U, 0x45U, 0x45U, 0x45U, 0x39U};
    static const uint8_t six[5] = {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U};
    static const uint8_t seven[5] = {0x01U, 0x71U, 0x09U, 0x05U, 0x03U};
    static const uint8_t eight[5] = {0x36U, 0x49U, 0x49U, 0x49U, 0x36U};
    static const uint8_t nine[5] = {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU};
    static const uint8_t a[5] = {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU};
    static const uint8_t b[5] = {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U};
    static const uint8_t c[5] = {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U};
    static const uint8_t g[5] = {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU};
    static const uint8_t i[5] = {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U};
    static const uint8_t m[5] = {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU};
    static const uint8_t n[5] = {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU};
    static const uint8_t r[5] = {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U};
    static const uint8_t s[5] = {0x46U, 0x49U, 0x49U, 0x49U, 0x31U};
    static const uint8_t t[5] = {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U};

    switch (ch) {
    case '-': return minus;
    case '/': return slash;
    case ':': return colon;
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '3': return three;
    case '4': return four;
    case '5': return five;
    case '6': return six;
    case '7': return seven;
    case '8': return eight;
    case '9': return nine;
    case 'A': return a;
    case 'B': return b;
    case 'C': return c;
    case 'G': return g;
    case 'I': return i;
    case 'M': return m;
    case 'N': return n;
    case 'R': return r;
    case 'S': return s;
    case 'T': return t;
    default: return blank;
    }
}

static void oled_write(const uint8_t control, const uint8_t *data, uint32_t len)
{
    uint8_t prefix = control;

    soft_iic_write_splicing_array(&g_oled_iic, &prefix, 1U, data, len);
}

static void oled_command(uint8_t command)
{
    oled_write(OLED_CONTROL_CMD, &command, 1U);
}

static void oled_set_cursor(uint8_t x, uint8_t page)
{
    if (x >= OLED_I2C_WIDTH) {
        x = OLED_I2C_WIDTH - 1U;
    }
    if (page >= OLED_I2C_PAGES) {
        page = OLED_I2C_PAGES - 1U;
    }

    oled_command((uint8_t)(0xB0U + page));
    oled_command((uint8_t)(0x00U + (x & 0x0FU)));
    oled_command((uint8_t)(0x10U + ((x >> 4U) & 0x0FU)));
}

void OledI2c_Init(void)
{
    static const uint8_t init_cmds[] = {
        0xAEU, 0x20U, 0x02U, 0xB0U, 0xC8U, 0x00U, 0x10U, 0x40U,
        0x81U, 0x7FU, 0xA1U, 0xA6U, 0xA8U, 0x3FU, 0xA4U, 0xD3U,
        0x00U, 0xD5U, 0x80U, 0xD9U, 0xF1U, 0xDAU, 0x12U, 0xDBU,
        0x40U, 0x8DU, 0x14U, 0xAFU
    };
    uint32_t i;

    soft_iic_init(&g_oled_iic, OLED_I2C_ADDR, OLED_I2C_DELAY,
                  APP_OLED_SOFT_IIC_SCL_PIN, APP_OLED_SOFT_IIC_SDA_PIN);
    system_delay_ms(50U);

    for (i = 0U; i < sizeof(init_cmds); ++i) {
        oled_command(init_cmds[i]);
    }
    OledI2c_Clear();
}

void OledI2c_Clear(void)
{
    uint8_t page;
    uint8_t col;
    uint8_t zeros[16] = {0U};

    for (page = 0U; page < OLED_I2C_PAGES; ++page) {
        oled_set_cursor(0U, page);
        for (col = 0U; col < OLED_I2C_WIDTH; col = (uint8_t)(col + sizeof(zeros))) {
            oled_write(OLED_CONTROL_DATA, zeros, sizeof(zeros));
        }
    }
}

void OledI2c_ShowString(uint8_t x, uint8_t page, const char *text)
{
    uint8_t glyph[6];
    const uint8_t *font;
    uint8_t i;

    while ((text != 0) && (*text != '\0') && (page < OLED_I2C_PAGES)) {
        if (x > (OLED_I2C_WIDTH - sizeof(glyph))) {
            break;
        }

        font = font5x7(*text);
        for (i = 0U; i < 5U; ++i) {
            glyph[i] = font[i];
        }
        glyph[5] = 0x00U;

        oled_set_cursor(x, page);
        oled_write(OLED_CONTROL_DATA, glyph, sizeof(glyph));
        x = (uint8_t)(x + sizeof(glyph));
        ++text;
    }
}

void OledI2c_ShowLargeString(uint8_t x, uint8_t page, const char *text,
                             uint8_t scale)
{
    uint8_t pages;
    uint8_t page_offset;
    uint8_t src_col;
    uint8_t src_row;
    uint8_t sx;
    uint8_t sy;
    uint8_t out_x;
    uint8_t dest_row;
    uint8_t column_pages[3];
    const uint8_t *font;

    if (scale < 2U) {
        scale = 2U;
    } else if (scale > 3U) {
        scale = 3U;
    }

    pages = (uint8_t)((7U * scale + 7U) / 8U);

    while ((text != 0) && (*text != '\0') && (page < OLED_I2C_PAGES)) {
        if (x > (OLED_I2C_WIDTH - (uint8_t)(6U * scale))) {
            break;
        }

        font = font5x7(*text);
        for (src_col = 0U; src_col < 6U; ++src_col) {
            column_pages[0] = 0U;
            column_pages[1] = 0U;
            column_pages[2] = 0U;

            if (src_col < 5U) {
                for (src_row = 0U; src_row < 7U; ++src_row) {
                    if ((font[src_col] & (uint8_t)(1U << src_row)) == 0U) {
                        continue;
                    }

                    for (sy = 0U; sy < scale; ++sy) {
                        dest_row = (uint8_t)(src_row * scale + sy);
                        column_pages[dest_row / 8U] |=
                            (uint8_t)(1U << (dest_row % 8U));
                    }
                }
            }

            for (sx = 0U; sx < scale; ++sx) {
                out_x = (uint8_t)(x + src_col * scale + sx);
                for (page_offset = 0U; page_offset < pages; ++page_offset) {
                    if ((uint8_t)(page + page_offset) >= OLED_I2C_PAGES) {
                        break;
                    }
                    oled_set_cursor(out_x, (uint8_t)(page + page_offset));
                    oled_write(OLED_CONTROL_DATA,
                               &column_pages[page_offset], 1U);
                }
            }
        }

        x = (uint8_t)(x + 6U * scale);
        ++text;
    }
}

void OledI2c_ShowInt(uint8_t x, uint8_t page, int32_t value, uint8_t width)
{
    char buffer[12];
    uint8_t pos = 0U;
    uint8_t out = 0U;
    uint32_t magnitude;
    char reversed[11];

    if (width > 10U) {
        width = 10U;
    }

    if (value < 0) {
        buffer[out++] = '-';
        magnitude = (uint32_t)(-value);
    } else {
        magnitude = (uint32_t)value;
    }

    do {
        reversed[pos++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while ((magnitude != 0U) && (pos < sizeof(reversed)));

    while ((width > pos) && ((out + pos) < (sizeof(buffer) - 1U))) {
        buffer[out++] = ' ';
        --width;
    }

    while ((pos > 0U) && (out < (sizeof(buffer) - 1U))) {
        buffer[out++] = reversed[--pos];
    }
    buffer[out] = '\0';
    OledI2c_ShowString(x, page, buffer);
}
