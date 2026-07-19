#include "vision_ascii_protocol.h"

#include <stdlib.h>
#include <string.h>

static uint8_t parse_i32(const char *text, int32_t min_value, int32_t max_value, int32_t *out)
{
    char *end = 0;
    long value;

    if (text == 0 || text[0] == '\0') {
        return 0U;
    }

    value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < min_value || value > max_value) {
        return 0U;
    }

    *out = (int32_t)value;
    return 1U;
}

static uint8_t parse_u32(const char *text, uint32_t max_value, uint32_t *out)
{
    int32_t value;

    if (!parse_i32(text, 0, (int32_t)max_value, &value)) {
        return 0U;
    }

    *out = (uint32_t)value;
    return 1U;
}

static uint8_t parse_decimal_0p1(const char *text, int32_t *out)
{
    int32_t sign = 1;
    int32_t integer = 0;
    int32_t fraction = 0;
    const char *cursor = text;

    if (cursor == 0 || *cursor == '\0') {
        return 0U;
    }

    if (*cursor == '-') {
        sign = -1;
        ++cursor;
    } else if (*cursor == '+') {
        ++cursor;
    }

    if (*cursor < '0' || *cursor > '9') {
        return 0U;
    }

    while (*cursor >= '0' && *cursor <= '9') {
        integer = integer * 10 + (*cursor - '0');
        ++cursor;
    }

    if (*cursor == '.') {
        ++cursor;
        if (*cursor >= '0' && *cursor <= '9') {
            fraction = *cursor - '0';
            ++cursor;
        }
        while (*cursor >= '0' && *cursor <= '9') {
            ++cursor;
        }
    }

    if (*cursor != '\0') {
        return 0U;
    }

    *out = sign * (integer * 10 + fraction);
    return 1U;
}

static uint8_t split_fields(char *frame, char *fields[], uint8_t max_fields)
{
    uint8_t count = 0U;
    char *cursor = frame;

    while (count < max_fields) {
        fields[count++] = cursor;
        while (*cursor != '\0' && *cursor != ',') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor = '\0';
        ++cursor;
    }

    return count;
}

static uint8_t parse_v1(char *fields[], uint8_t count, VisionAscii_Result *result)
{
    uint32_t value_u32;
    int32_t value_i32;

    if (count != 12U) {
        return 0U;
    }

    if (!parse_u32(fields[1], 255U, &value_u32)) return 0U;
    result->version = (uint8_t)value_u32;
    if (!parse_u32(fields[2], 65535U, &value_u32)) return 0U;
    result->sequence = (uint16_t)value_u32;
    if (!parse_u32(fields[3], 255U, &value_u32)) return 0U;
    result->mode = (uint8_t)value_u32;
    if (!parse_i32(fields[4], -32768, 32767, &value_i32)) return 0U;
    result->cx = (int16_t)value_i32;
    if (!parse_i32(fields[5], -32768, 32767, &value_i32)) return 0U;
    result->cy = (int16_t)value_i32;
    if (!parse_u32(fields[6], 65535U, &value_u32)) return 0U;
    result->width = (uint16_t)value_u32;
    if (!parse_u32(fields[7], 65535U, &value_u32)) return 0U;
    result->height = (uint16_t)value_u32;
    if (!parse_i32(fields[8], -2147483647 - 1, 2147483647, &result->distance_0p1cm)) return 0U;
    if (!parse_i32(fields[9], -2147483647 - 1, 2147483647, &result->size_0p1cm)) return 0U;
    if (!parse_i32(fields[10], -32768, 32767, &value_i32)) return 0U;
    result->angle_0p1deg = (int16_t)value_i32;
    if (!parse_u32(fields[11], 1U, &value_u32)) return 0U;
    result->valid = (result->mode != 0U && value_u32 != 0U) ? 1U : 0U;

    return 1U;
}

static uint8_t parse_legacy(char *fields[], uint8_t count, VisionAscii_Result *result)
{
    uint32_t value_u32;
    int32_t value_i32;

    if (count != 10U) {
        return 0U;
    }

    result->version = 0U;
    result->sequence = (uint16_t)(result->sequence + 1U);
    if (!parse_u32(fields[1], 255U, &value_u32)) return 0U;
    result->mode = (uint8_t)value_u32;
    if (!parse_i32(fields[2], -32768, 32767, &value_i32)) return 0U;
    result->cx = (int16_t)value_i32;
    if (!parse_i32(fields[3], -32768, 32767, &value_i32)) return 0U;
    result->cy = (int16_t)value_i32;
    if (!parse_u32(fields[4], 65535U, &value_u32)) return 0U;
    result->width = (uint16_t)value_u32;
    if (!parse_u32(fields[5], 65535U, &value_u32)) return 0U;
    result->height = (uint16_t)value_u32;
    if (!parse_decimal_0p1(fields[6], &result->distance_0p1cm)) return 0U;
    if (!parse_decimal_0p1(fields[7], &result->size_0p1cm)) return 0U;
    if (!parse_decimal_0p1(fields[8], &value_i32)) return 0U;
    result->angle_0p1deg = (int16_t)value_i32;
    if (!parse_decimal_0p1(fields[9], &value_i32)) return 0U;
    result->valid = (result->mode != 0U && value_i32 >= 5) ? 1U : 0U;

    return 1U;
}

static uint8_t parse_frame(VisionAscii_Parser *parser)
{
    char frame[VISION_ASCII_MAX_FRAME_LEN];
    char *fields[12] = {0};
    uint8_t count;
    VisionAscii_Result result = parser->result;

    if (parser->length < 4U || parser->buffer[0] != '$' ||
        parser->buffer[1] != 'V' || parser->buffer[2] != ',') {
        return 0U;
    }

    memcpy(frame, parser->buffer, parser->length);
    frame[parser->length - 1U] = '\0';
    count = split_fields(&frame[1], fields, 12U);

    if (strcmp(fields[0], "V") != 0) {
        return 0U;
    }

    if (parse_v1(fields, count, &result) || parse_legacy(fields, count, &result)) {
        parser->result = result;
        return 1U;
    }

    return 0U;
}

void VisionAscii_Init(VisionAscii_Parser *parser)
{
    if (parser == 0) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
}

uint8_t VisionAscii_InputByte(VisionAscii_Parser *parser, uint8_t byte)
{
    if (parser == 0) {
        return 0U;
    }

    if (byte == '$') {
        parser->receiving = 1U;
        parser->length = 0U;
    }

    if (!parser->receiving) {
        return 0U;
    }

    if (parser->length >= VISION_ASCII_MAX_FRAME_LEN) {
        parser->receiving = 0U;
        parser->length = 0U;
        parser->parse_error = 1U;
        return 0U;
    }

    parser->buffer[parser->length++] = (char)byte;
    if (byte != '#') {
        return 0U;
    }

    parser->receiving = 0U;
    parser->frame_ready = parse_frame(parser);
    parser->parse_error = parser->frame_ready ? 0U : 1U;
    parser->length = 0U;
    return parser->frame_ready;
}

uint8_t VisionAscii_TakeResult(VisionAscii_Parser *parser, VisionAscii_Result *result)
{
    if (parser == 0 || result == 0 || parser->frame_ready == 0U) {
        return 0U;
    }

    *result = parser->result;
    parser->frame_ready = 0U;
    return 1U;
}
