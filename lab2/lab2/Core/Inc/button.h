#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

typedef enum {
    BTN_NONE = 0,
    BTN_SHORT_PRESS,
    BTN_LONG_PRESS
} button_event_t;

void button_init(void);
button_event_t button_poll(void);

#endif
