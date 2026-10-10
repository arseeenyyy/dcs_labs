#include "button.h"
#include "main.h"

#define BUTTON_PORT         GPIOC
#define PIN_BUTTON          GPIO_PIN_15

#define BUTTON_DEBOUNCE_MS  30u
#define BUTTON_LONG_MS      500u

static uint8_t  s_last_raw;
static uint8_t  s_stable;
static uint32_t s_raw_change_ms;
static uint32_t s_press_ms;
static uint8_t  s_press_active;

static uint8_t btn_read_raw(void)
{
    return (HAL_GPIO_ReadPin(BUTTON_PORT, PIN_BUTTON) == GPIO_PIN_RESET) ? 1u : 0u;
}

void button_init(void)
{
    s_last_raw      = btn_read_raw();
    s_stable        = s_last_raw;
    s_raw_change_ms = HAL_GetTick();
    s_press_active  = 0u;
}

button_event_t button_poll(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t  raw = btn_read_raw();

    if (raw != s_last_raw) {
        s_last_raw      = raw;
        s_raw_change_ms = now;
    }

    if ((now - s_raw_change_ms) < BUTTON_DEBOUNCE_MS) return BTN_NONE;
    if (raw == s_stable) return BTN_NONE;

    s_stable = raw;

    if (s_stable == 1u) {
        s_press_ms     = now;
        s_press_active = 1u;
        return BTN_NONE;
    } else {
        if (s_press_active) {
            uint32_t dur = now - s_press_ms;
            s_press_active = 0u;
            return (dur >= BUTTON_LONG_MS) ? BTN_LONG_PRESS : BTN_SHORT_PRESS;
        }
        return BTN_NONE;
    }
}
