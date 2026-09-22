#include "tjc_touch.h"

#include <stddef.h>

#define TJC_TOUCH_FRAME_HEADER        0x65U
#define TJC_TOUCH_FRAME_LENGTH        7U
#define TJC_TOUCH_RELEASE_EVENT       0x01U
#define TJC_TOUCH_END_BYTE            0xFFU

#define TJC_PAGE_RUN                  0U
#define TJC_PAGE_DEBUG                1U
#define TJC_COMPONENT_START           11U
#define TJC_COMPONENT_STOP            12U
#define TJC_COMPONENT_DEBUG           13U
#define TJC_COMPONENT_BACK            11U

static TJC_TouchEvent decode_touch_event(const TJC_TouchParser *parser)
{
    const uint8_t page = parser->frame[1];
    const uint8_t component = parser->frame[2];
    const uint8_t event = parser->frame[3];

    if (event != TJC_TOUCH_RELEASE_EVENT) {
        return TJC_TOUCH_EVENT_NONE;
    }

    if (page == TJC_PAGE_RUN) {
        if (component == TJC_COMPONENT_START) {
            return TJC_TOUCH_EVENT_START_REQUEST;
        }
        if (component == TJC_COMPONENT_STOP) {
            return TJC_TOUCH_EVENT_STOP_REQUEST;
        }
        if (component == TJC_COMPONENT_DEBUG) {
            return TJC_TOUCH_EVENT_SHOW_DEBUG;
        }
    }

    if (page == TJC_PAGE_DEBUG && component == TJC_COMPONENT_BACK) {
        return TJC_TOUCH_EVENT_SHOW_RUN;
    }

    return TJC_TOUCH_EVENT_NONE;
}

void TJC_TouchInit(TJC_TouchParser *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->length = 0U;
}

bool TJC_TouchInputByte(TJC_TouchParser *parser, uint8_t byte,
                        TJC_TouchEvent *event)
{
    TJC_TouchEvent decoded;

    if (parser == NULL || event == NULL) {
        return false;
    }

    *event = TJC_TOUCH_EVENT_NONE;

    if (parser->length == 0U) {
        if (byte == TJC_TOUCH_FRAME_HEADER) {
            parser->frame[0] = byte;
            parser->length = 1U;
        }
        return false;
    }

    if (byte == TJC_TOUCH_FRAME_HEADER) {
        parser->frame[0] = byte;
        parser->length = 1U;
        return false;
    }

    parser->frame[parser->length++] = byte;
    if (parser->length < TJC_TOUCH_FRAME_LENGTH) {
        return false;
    }

    parser->length = 0U;
    if (parser->frame[4] != TJC_TOUCH_END_BYTE ||
        parser->frame[5] != TJC_TOUCH_END_BYTE ||
        parser->frame[6] != TJC_TOUCH_END_BYTE) {
        return false;
    }

    decoded = decode_touch_event(parser);
    if (decoded == TJC_TOUCH_EVENT_NONE) {
        return false;
    }

    *event = decoded;
    return true;
}

const char *TJC_TouchEventName(TJC_TouchEvent event)
{
    switch (event) {
    case TJC_TOUCH_EVENT_START_REQUEST:
        return "START";
    case TJC_TOUCH_EVENT_STOP_REQUEST:
        return "STOP";
    case TJC_TOUCH_EVENT_SHOW_DEBUG:
        return "DEBUG";
    case TJC_TOUCH_EVENT_SHOW_RUN:
        return "RUN";
    case TJC_TOUCH_EVENT_NONE:
    default:
        return "NONE";
    }
}
