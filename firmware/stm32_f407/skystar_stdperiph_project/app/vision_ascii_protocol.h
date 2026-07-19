#ifndef VISION_ASCII_PROTOCOL_H
#define VISION_ASCII_PROTOCOL_H

#include <stdint.h>

#define VISION_ASCII_MAX_FRAME_LEN 128U
#define VISION_ASCII_TIMEOUT_MS 500U

typedef struct {
    uint8_t version;
    uint16_t sequence;
    uint8_t mode;
    int16_t cx;
    int16_t cy;
    uint16_t width;
    uint16_t height;
    int32_t distance_0p1cm;
    int32_t size_0p1cm;
    int16_t angle_0p1deg;
    uint8_t valid;
} VisionAscii_Result;

typedef struct {
    char buffer[VISION_ASCII_MAX_FRAME_LEN];
    uint16_t length;
    uint8_t receiving;
    uint8_t frame_ready;
    uint8_t parse_error;
    VisionAscii_Result result;
} VisionAscii_Parser;

void VisionAscii_Init(VisionAscii_Parser *parser);
uint8_t VisionAscii_InputByte(VisionAscii_Parser *parser, uint8_t byte);
uint8_t VisionAscii_TakeResult(VisionAscii_Parser *parser, VisionAscii_Result *result);

#endif
