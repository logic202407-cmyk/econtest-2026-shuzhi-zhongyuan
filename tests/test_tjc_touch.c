#include <assert.h>
#include <stdint.h>

#include "../application/display/tjc_screen/tjc_touch.h"

static TJC_TouchEvent feed(const uint8_t *frame, uint8_t length)
{
    TJC_TouchParser parser;
    TJC_TouchEvent event = TJC_TOUCH_EVENT_NONE;
    uint8_t index;

    TJC_TouchInit(&parser);
    for (index = 0U; index < length; ++index) {
        (void)TJC_TouchInputByte(&parser, frame[index], &event);
    }
    return event;
}

int main(void)
{
    static const uint8_t start_release[] = {0x65U, 0U, 11U, 1U, 0xFFU, 0xFFU, 0xFFU};
    static const uint8_t stop_release[] = {0x65U, 0U, 12U, 1U, 0xFFU, 0xFFU, 0xFFU};
    static const uint8_t start_press[] = {0x65U, 0U, 11U, 0U, 0xFFU, 0xFFU, 0xFFU};
    static const uint8_t bad_tail[] = {0x65U, 0U, 11U, 1U, 0xFFU, 0xFFU, 0U};

    assert(feed(start_release, sizeof(start_release)) == TJC_TOUCH_EVENT_START_REQUEST);
    assert(feed(stop_release, sizeof(stop_release)) == TJC_TOUCH_EVENT_STOP_REQUEST);
    assert(feed(start_press, sizeof(start_press)) == TJC_TOUCH_EVENT_NONE);
    assert(feed(bad_tail, sizeof(bad_tail)) == TJC_TOUCH_EVENT_NONE);
    return 0;
}
