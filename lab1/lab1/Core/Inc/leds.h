#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

/* Битовая маска светодиодов */
#define LED_GREEN   0x01u
#define LED_YELLOW  0x02u
#define LED_ALL     (LED_GREEN | LED_YELLOW)

void leds_init(void);
void leds_write(uint8_t mask);   /* 1 в бите = зажечь, 0 = погасить */
uint8_t leds_read(void);          /* текущее записанное состояние */

#endif
