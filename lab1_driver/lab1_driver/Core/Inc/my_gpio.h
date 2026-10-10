#ifndef MY_GPIO_H
#define MY_GPIO_H

#include <stdint.h>
#include "stm32f4xx.h"

// moder
typedef enum {
    MY_GPIO_MODE_INPUT  = 0x00u, //in
    MY_GPIO_MODE_OUTPUT = 0x01u, //out
    MY_GPIO_MODE_AF     = 0x02u, //alternate function
    MY_GPIO_MODE_ANALOG = 0x03u // analog
} my_gpio_mode_t;

// otyper
typedef enum {
    MY_GPIO_OTYPE_PP = 0x0u,  /* Push-pull  */
    MY_GPIO_OTYPE_OD = 0x1u   /* Open-drain */
} my_gpio_otype_t;

// speedr
typedef enum {
    MY_GPIO_SPEED_LOW       = 0x0u,
    MY_GPIO_SPEED_MEDIUM    = 0x1u,
    MY_GPIO_SPEED_HIGH      = 0x2u,
    MY_GPIO_SPEED_VERY_HIGH = 0x3u
} my_gpio_speed_t;

// pupdr
typedef enum {
    MY_GPIO_PULL_NONE = 0x0u,
    MY_GPIO_PULL_UP   = 0x1u,
    MY_GPIO_PULL_DOWN = 0x2u
} my_gpio_pull_t;

// port - pointer to struct of registers
typedef GPIO_TypeDef* my_gpio_port_t;

// enable clocking on port
void my_gpio_enable_clock(my_gpio_port_t port);


// init one pin(0...15): -1 if error
int my_gpio_init(my_gpio_port_t port, uint8_t pin,
                 my_gpio_mode_t mode,
                 my_gpio_otype_t otype,
                 my_gpio_speed_t speed,
                 my_gpio_pull_t pull);

// read pin configuration
int my_gpio_get_config(my_gpio_port_t port, uint8_t pin,
                       my_gpio_mode_t  *mode,
                       my_gpio_otype_t *otype,
                       my_gpio_speed_t *speed,
                       my_gpio_pull_t  *pull);

// write pin: value = 0 or 1
void my_gpio_write_pin(my_gpio_port_t port, uint8_t pin, uint8_t value);

// read 1 pin: 0 or 1
uint8_t my_gpio_read_pin(my_gpio_port_t port, uint8_t pin);

// write few pins of port
void my_gpio_write_port(my_gpio_port_t port, uint16_t mask);

// read all pins on port
uint16_t my_gpio_read_port(my_gpio_port_t port);

#endif /* MY_GPIO_H */
