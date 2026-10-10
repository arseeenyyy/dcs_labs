#include "my_gpio.h"

// valid pin value
static int pin_is_valid(uint8_t pin)
{
    return (pin <= 15u);
}

// pointer on port compare with bit value: RCC -> AHB1ENR
static uint32_t rcc_bit_for_port(my_gpio_port_t port)
{
    if (port == GPIOA) return (1u << 0);
    if (port == GPIOB) return (1u << 1);
    if (port == GPIOC) return (1u << 2);
    if (port == GPIOD) return (1u << 3);
    if (port == GPIOE) return (1u << 4);
    if (port == GPIOF) return (1u << 5);
    if (port == GPIOG) return (1u << 6);
    if (port == GPIOH) return (1u << 7);
    if (port == GPIOI) return (1u << 8);
    if (port == GPIOJ) return (1u << 9);
    if (port == GPIOK) return (1u << 10);
    return 0u;
}

// enable clocking on port
void my_gpio_enable_clock(my_gpio_port_t port)
{
    uint32_t bit = rcc_bit_for_port(port);
    if (bit != 0u) {
        RCC->AHB1ENR |= bit;
        (void)RCC->AHB1ENR;
    }
}
// init function
int my_gpio_init(my_gpio_port_t port, uint8_t pin,
                 my_gpio_mode_t mode,
                 my_gpio_otype_t otype,
                 my_gpio_speed_t speed,
                 my_gpio_pull_t pull)
{
    if ((port == 0) || !pin_is_valid(pin)) {
        return -1;
    }

    uint32_t tmp;

    // OTYPER
    tmp  = port->OTYPER;
    tmp &= ~(1u << pin);
    tmp |= ((uint32_t)otype << pin);
    port->OTYPER = tmp;

    // OSPEEDR
    tmp  = port->OSPEEDR;
    tmp &= ~(3u << (pin * 2u));
    tmp |= ((uint32_t)speed << (pin * 2u));
    port->OSPEEDR = tmp;

    // PUPDR
    tmp  = port->PUPDR;
    tmp &= ~(3u << (pin * 2u));
    tmp |= ((uint32_t)pull << (pin * 2u));
    port->PUPDR = tmp;

    // for output reseting out before enable out(for avoiding high signal after switching MODER)
    if (mode == MY_GPIO_MODE_OUTPUT) {
        port->BSRR = (1u << (pin + 16u));  // reseting ODR
    }

    // MODER
    tmp  = port->MODER;
    tmp &= ~(3u << (pin * 2u));
    tmp |= ((uint32_t)mode << (pin * 2u));
    port->MODER = tmp;

    return 0;
}

int my_gpio_get_config(my_gpio_port_t port, uint8_t pin,
                       my_gpio_mode_t  *mode,
                       my_gpio_otype_t *otype,
                       my_gpio_speed_t *speed,
                       my_gpio_pull_t  *pull)
{
    if ((port == 0) || !pin_is_valid(pin)) return -1;

    if (mode)  *mode  = (my_gpio_mode_t )((port->MODER   >> (pin * 2u)) & 0x3u);
    if (otype) *otype = (my_gpio_otype_t)((port->OTYPER  >>  pin       ) & 0x1u);
    if (speed) *speed = (my_gpio_speed_t)((port->OSPEEDR >> (pin * 2u)) & 0x3u);
    if (pull)  *pull  = (my_gpio_pull_t )((port->PUPDR   >> (pin * 2u)) & 0x3u);

    return 0;
}

void my_gpio_write_pin(my_gpio_port_t port, uint8_t pin, uint8_t value)
{
    if ((port == 0) || !pin_is_valid(pin)) return;


    // BSRR: low 16 bits - set,
    // high 16 bits - reset
    if (value) {
        port->BSRR = (1u << pin);
    } else {
        port->BSRR = (1u << (pin + 16u));
    }
}

uint8_t my_gpio_read_pin(my_gpio_port_t port, uint8_t pin)
{
    if ((port == 0) || !pin_is_valid(pin)) return 0;
    return (uint8_t)((port->IDR >> pin) & 0x1u);
}

void my_gpio_write_port(my_gpio_port_t port, uint16_t mask)
{
    if (port == 0) return;
    port->ODR = (uint32_t)mask;
}

uint16_t my_gpio_read_port(my_gpio_port_t port)
{
    if (port == 0) return 0;
    return (uint16_t)(port->IDR & 0xFFFFu);
}
