#include "button.h"
#include "main.h"

#define BUTTON_PORT         GPIOC
#define PIN_BUTTON          GPIO_PIN_15

#define BUTTON_DEBOUNCE_MS  30u    /* сигнал должен быть стабильным */
#define BUTTON_LONG_MS      500u   /* порог «долгого» нажатия */

static uint8_t  s_last_raw;        /* последнее сырое состояние */
static uint8_t  s_stable;          /* стабильное (после фильтра) */
static uint32_t s_raw_change_ms;   /* когда сырое последний раз менялось */
static uint32_t s_press_ms;        /* когда зафиксировали стабильное нажатие */
static uint8_t  s_press_active;    /* идёт ли нажатие прямо сейчас */

/* 1 = нажата (активный низкий: 0 на пине) */
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

    /* Зафиксировали изменение сырого состояния — запомнили время */
    if (raw != s_last_raw) {
        s_last_raw      = raw;
        s_raw_change_ms = now;
    }

    /* Пока не прошёл интервал антидребезга — молчим */
    if ((now - s_raw_change_ms) < BUTTON_DEBOUNCE_MS) {
        return BTN_NONE;
    }

    /* Стабильное состояние не изменилось — событий нет */
    if (raw == s_stable) {
        return BTN_NONE;
    }

    /* Зафиксировали новое стабильное состояние */
    s_stable = raw;

    if (s_stable == 1u) {
        /* Только что нажали — запомнили время */
        s_press_ms     = now;
        s_press_active = 1u;
        return BTN_NONE;
    } else {
        /* Только что отпустили — классифицируем по длительности */
        if (s_press_active) {
            uint32_t dur = now - s_press_ms;
            s_press_active = 0u;
            return (dur >= BUTTON_LONG_MS) ? BTN_LONG_PRESS : BTN_SHORT_PRESS;
        }
        return BTN_NONE;
    }
}
