#include "uart_drv.h"
#include "ring.h"
#include "main.h"

extern UART_HandleTypeDef huart6;
#define UART_HANDLE   (&huart6)
#define UART_IRQn     USART6_IRQn

static uint8_t s_tx_storage[UART_TX_QUEUE_SIZE];
static uint8_t s_rx_storage[UART_RX_QUEUE_SIZE];
static ring_t  s_txq;
static ring_t  s_rxq;

static uart_mode_t      s_mode = UART_MODE_POLL;
static uint8_t          s_rx_it_byte;
static uint8_t          s_tx_it_byte;
static volatile uint8_t s_tx_busy;

void uart_init(void)
{
    ring_init(&s_txq, s_tx_storage, sizeof(s_tx_storage));
    ring_init(&s_rxq, s_rx_storage, sizeof(s_rx_storage));
    s_mode    = UART_MODE_POLL;
    s_tx_busy = 0;
}

void uart_set_mode(uart_mode_t mode)
{
    HAL_NVIC_DisableIRQ(UART_IRQn);
    HAL_UART_Abort_IT(UART_HANDLE);

    ring_clear(&s_txq);
    ring_clear(&s_rxq);
    s_tx_busy = 0;

    s_mode = mode;

    if (mode == UART_MODE_IRQ) {
        HAL_UART_Receive_IT(UART_HANDLE, &s_rx_it_byte, 1);
        HAL_NVIC_EnableIRQ(UART_IRQn);
    }
}

uart_mode_t uart_get_mode(void)
{
    return s_mode;
}

int uart_send_byte(uint8_t b)
{
    if (s_mode == UART_MODE_IRQ) {
        if (!ring_push(&s_txq, b)) return 0;
        if (!s_tx_busy) {
            if (ring_pop(&s_txq, &s_tx_it_byte)) {
                s_tx_busy = 1;
                if (HAL_UART_Transmit_IT(UART_HANDLE, &s_tx_it_byte, 1) != HAL_OK) {
                    s_tx_busy = 0;
                    return 0;
                }
            }
        }
        return 1;
    } else {
        return (HAL_UART_Transmit(UART_HANDLE, &b, 1, 5) == HAL_OK) ? 1 : 0;
    }
}

int uart_send_str(const char *s)
{
    int n = 0;
    while (*s) {
        if (!uart_send_byte((uint8_t)*s)) break;
        s++;
        n++;
    }
    return n;
}

int uart_recv_byte(uint8_t *b)
{
    if (s_mode == UART_MODE_IRQ) {
        return ring_pop(&s_rxq, b);
    } else {
        HAL_StatusTypeDef st = HAL_UART_Receive(UART_HANDLE, b, 1, 0);
        return (st == HAL_OK) ? 1 : 0;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != UART_HANDLE) return;
    ring_push(&s_rxq, s_rx_it_byte);
    HAL_UART_Receive_IT(UART_HANDLE, &s_rx_it_byte, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != UART_HANDLE) return;
    if (ring_pop(&s_txq, &s_tx_it_byte)) {
        HAL_UART_Transmit_IT(UART_HANDLE, &s_tx_it_byte, 1);
    } else {
        s_tx_busy = 0;
    }
}
