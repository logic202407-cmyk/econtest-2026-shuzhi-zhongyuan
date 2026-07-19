#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../firmware/stm32_f407/skystar_stdperiph_project/app/vision_ascii_protocol.h"

static uint8_t feed(VisionAscii_Parser *parser, const char *text)
{
    uint8_t accepted = 0U;
    size_t index;

    for (index = 0U; index < strlen(text); ++index) {
        accepted = VisionAscii_InputByte(parser, (uint8_t)text[index]) || accepted;
    }
    return accepted;
}

static void test_v1_frame(void)
{
    VisionAscii_Parser parser;
    VisionAscii_Result result;

    VisionAscii_Init(&parser);
    assert(feed(&parser, "$V,1,25,1,320,240,80,80,1500,80,-5,1#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.version == 1U);
    assert(result.sequence == 25U);
    assert(result.mode == 1U);
    assert(result.cx == 320);
    assert(result.cy == 240);
    assert(result.width == 80U);
    assert(result.height == 80U);
    assert(result.distance_0p1cm == 1500);
    assert(result.size_0p1cm == 80);
    assert(result.angle_0p1deg == -5);
    assert(result.valid == 1U);
}

static void test_legacy_fake_data(void)
{
    VisionAscii_Parser parser;
    VisionAscii_Result result;

    VisionAscii_Init(&parser);
    assert(feed(&parser, "$V,1,320,240,80,80,150.0,8.0,0.0,0.99#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.version == 0U);
    assert(result.sequence == 1U);
    assert(result.mode == 1U);
    assert(result.distance_0p1cm == 1500);
    assert(result.size_0p1cm == 80);
    assert(result.angle_0p1deg == 0);
    assert(result.valid == 1U);
}

static void test_lost_frames(void)
{
    VisionAscii_Parser parser;
    VisionAscii_Result result;

    VisionAscii_Init(&parser);
    assert(feed(&parser, "$V,0,320,240,80,80,150.0,8.0,0.0,0.10#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.mode == 0U);
    assert(result.valid == 0U);

    assert(feed(&parser, "$V,1,320,240,80,80,150.0,8.0,0.0,0.49#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.mode == 1U);
    assert(result.valid == 0U);

    assert(feed(&parser, "$V,1,26,0,322,239,82,79,1498,81,12,1#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.mode == 0U);
    assert(result.valid == 0U);
}

static void test_resync_and_bad_frame(void)
{
    VisionAscii_Parser parser;
    VisionAscii_Result result;

    VisionAscii_Init(&parser);
    assert(!feed(&parser, "noise$V,1,2,bad#"));
    assert(parser.parse_error == 1U);
    assert(feed(&parser, "xx$V,1,26,1,322,239,82,79,1498,81,12,1#"));
    assert(VisionAscii_TakeResult(&parser, &result));
    assert(result.sequence == 26U);
    assert(result.cx == 322);
    assert(result.cy == 239);
    assert(result.angle_0p1deg == 12);
}

int main(void)
{
    test_v1_frame();
    test_legacy_fake_data();
    test_lost_frames();
    test_resync_and_bad_frame();
    return 0;
}
