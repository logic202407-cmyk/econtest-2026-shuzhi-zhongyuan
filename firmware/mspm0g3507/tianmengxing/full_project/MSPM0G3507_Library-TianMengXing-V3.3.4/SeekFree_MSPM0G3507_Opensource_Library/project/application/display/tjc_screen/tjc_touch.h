#ifndef TJC_TOUCH_H
#define TJC_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TJC_TOUCH_EVENT_NONE = 0,
    TJC_TOUCH_EVENT_START_REQUEST,
    TJC_TOUCH_EVENT_STOP_REQUEST,
    TJC_TOUCH_EVENT_SHOW_DEBUG,
    TJC_TOUCH_EVENT_SHOW_RUN
} TJC_TouchEvent;

typedef struct
{
    uint8_t frame[7];
    uint8_t length;
} TJC_TouchParser;

// Parse the standard TJC component-ID return frame:
// 65 <page_id> <component_id> <event> FF FF FF
// Only touch-release events are exposed to the application.
void TJC_TouchInit(TJC_TouchParser *parser);
bool TJC_TouchInputByte(TJC_TouchParser *parser, uint8_t byte,
                        TJC_TouchEvent *event);
const char *TJC_TouchEventName(TJC_TouchEvent event);

#ifdef __cplusplus
}
#endif

#endif
