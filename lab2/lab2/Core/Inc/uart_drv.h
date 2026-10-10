#ifndef UART_DRV_H
#define UART_DRV_H

#include <stdint.h>

#ifndef UART_TX_QUEUE_SIZE
#define UART_TX_QUEUE_SIZE 128u
#endif
#ifndef UART_RX_QUEUE_SIZE
#define UART_RX_QUEUE_SIZE 128u
#endif

typedef enum {
    UART_MODE_POLL = 0,
    UART_MODE_IRQ  = 1
} uart_mode_t;

void        uart_init(void);
void        uart_set_mode(uart_mode_t mode);
uart_mode_t uart_get_mode(void);

int  uart_send_byte(uint8_t b);
int  uart_send_str(const char *s);
int  uart_recv_byte(uint8_t *b);

#endif
