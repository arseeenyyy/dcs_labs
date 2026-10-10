#include "leds.h"
#include "main.h"

/* Пины и порт спрятаны внутри драйвера — приложение их не видит */
#define LED_PORT      GPIOD
#define PIN_GREEN     GPIO_PIN_13
#define PIN_YELLOW    GPIO_PIN_14

void leds_init(void)
{
    leds_write(0);
}

void leds_write(uint8_t mask)
{
    HAL_GPIO_WritePin(LED_PORT, PIN_GREEN,
                      (mask & LED_GREEN)  ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, PIN_YELLOW,
                      (mask & LED_YELLOW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t leds_read(void)
{
    uint8_t v = 0;
    if (HAL_GPIO_ReadPin(LED_PORT, PIN_GREEN)  == GPIO_PIN_SET) v |= LED_GREEN;
    if (HAL_GPIO_ReadPin(LED_PORT, PIN_YELLOW) == GPIO_PIN_SET) v |= LED_YELLOW;
    return v;
}
