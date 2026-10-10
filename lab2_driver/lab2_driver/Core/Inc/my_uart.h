#ifndef MY_UART_H
#define MY_UART_H

#include <stdint.h>

/* Размеры очередей — настраиваются на этапе компиляции */
#ifndef MY_UART_TX_QUEUE_SIZE
#define MY_UART_TX_QUEUE_SIZE 128u
#endif
#ifndef MY_UART_RX_QUEUE_SIZE
#define MY_UART_RX_QUEUE_SIZE 128u
#endif

typedef enum {
    MY_UART_MODE_POLL = 0,
    MY_UART_MODE_IRQ  = 1
} my_uart_mode_t;

/* Инициализация. pclk2 — частота APB2 в герцах (для расчёта BRR). */
void my_uart_init(uint32_t pclk2, uint32_t baud);

/* Смена режима: POLL / IRQ. Очищает очереди. */
void my_uart_set_mode(my_uart_mode_t mode);
my_uart_mode_t my_uart_get_mode(void);

/* Отправка (неблокирующая в IRQ-режиме). 1 = ок, 0 = очередь переполнена. */
int my_uart_send_byte(uint8_t b);
int my_uart_send_str(const char *s);

/* Приём одного байта (неблокирующий).
   1 = байт принят в *b, 0 = нет данных. */
int my_uart_recv_byte(uint8_t *b);

/* Вызывать из USART6_IRQHandler. */
void my_uart_irq_handler(void);

#endif
