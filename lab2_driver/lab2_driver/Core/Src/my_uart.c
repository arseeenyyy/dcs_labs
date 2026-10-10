#include "my_uart.h"
#include "ring.h"
#include "stm32f4xx.h"

/* ============================================================
   НАСТРОЙКИ ПОД ПРОЕКТ
   ============================================================ */
#define MY_UART        USART6
#define MY_UART_IRQn   USART6_IRQn

/* Пины USART6. Проверь по схеме стенда!
   На STM32F427 USART6_TX/RX могут быть на PC6/PC7 (AF8) или PG14/PG9 (AF8). */
#define MY_UART_GPIO_PORT   GPIOC
#define MY_UART_GPIO_EN     RCC_AHB1ENR_GPIOCEN
#define MY_UART_TX_PIN      6u     /* PC6 */
#define MY_UART_RX_PIN      7u     /* PC7 */
#define MY_UART_AF          8u     /* AF8 = USART6 */
/* ============================================================ */

/* ---- Очереди ---- */
static uint8_t s_tx_storage[MY_UART_TX_QUEUE_SIZE];
static uint8_t s_rx_storage[MY_UART_RX_QUEUE_SIZE];
static ring_t  s_txq;
static ring_t  s_rxq;

/* ---- Состояние ---- */
static my_uart_mode_t   s_mode;
static volatile uint8_t s_tx_busy;

/* ============================================================
   Вспомогательные функции
   ============================================================ */

static void gpio_set_af(uint8_t pin, uint8_t af)
{
    uint32_t tmp;

    /* MODER = 10 (AF) */
    tmp = MY_UART_GPIO_PORT->MODER;
    tmp &= ~(3u << (pin * 2u));
    tmp |=  (2u << (pin * 2u));
    MY_UART_GPIO_PORT->MODER = tmp;

    /* OTYPER = 0 (push-pull) */
    tmp = MY_UART_GPIO_PORT->OTYPER;
    tmp &= ~(1u << pin);
    MY_UART_GPIO_PORT->OTYPER = tmp;

    /* OSPEEDR = 10 (high) — для UART нужен быстрый фронт */
    tmp = MY_UART_GPIO_PORT->OSPEEDR;
    tmp &= ~(3u << (pin * 2u));
    tmp |=  (2u << (pin * 2u));
    MY_UART_GPIO_PORT->OSPEEDR = tmp;

    /* PUPDR = 01 (pull-up) — линия не должна висеть */
    tmp = MY_UART_GPIO_PORT->PUPDR;
    tmp &= ~(3u << (pin * 2u));
    tmp |=  (1u << (pin * 2u));
    MY_UART_GPIO_PORT->PUPDR = tmp;

    /* AFR: 4 бита на пин. AFR[0] для пинов 0..7, AFR[1] для 8..15. */
    uint32_t idx = pin / 8u;
    uint32_t pos = (pin % 8u) * 4u;
    tmp = MY_UART_GPIO_PORT->AFR[idx];
    tmp &= ~(0xFu << pos);
    tmp |=  ((uint32_t)af << pos);
    MY_UART_GPIO_PORT->AFR[idx] = tmp;
}

static void nvic_enable_irq(IRQn_Type irqn)
{
    NVIC->ISER[((uint32_t)irqn) >> 5u] = (1u << (((uint32_t)irqn) & 0x1Fu));
}
static void nvic_disable_irq(IRQn_Type irqn)
{
    NVIC->ICER[((uint32_t)irqn) >> 5u] = (1u << (((uint32_t)irqn) & 0x1Fu));
}

/* ============================================================
   Публичные функции
   ============================================================ */

void my_uart_init(uint32_t pclk2, uint32_t baud)
{
    /* 1. Тактирование GPIO и USART6 */
    RCC->AHB1ENR |= MY_UART_GPIO_EN;
    (void)RCC->AHB1ENR;

    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    (void)RCC->APB2ENR;

    /* 2. Пины в режим AF8 */
    gpio_set_af(MY_UART_TX_PIN, MY_UART_AF);
    gpio_set_af(MY_UART_RX_PIN, MY_UART_AF);

    /* 3. Настройка USART6.
       Сначала выключаем UE, чтобы менять регистры безопасно. */
    MY_UART->CR1 = 0;
    MY_UART->CR2 = 0;
    MY_UART->CR3 = 0;

    /* CR2: 1 stop bit (00), clock не нужен, поля по умолчанию — 0 */

    /* BRR: при OVER8=0 (по умолчанию): USARTDIV = fCK / (16*baud).
       Значение BRR = USARTDIV * 16 = fCK / baud (с округлением). */
    MY_UART->BRR = (pclk2 + baud / 2u) / baud;

    /* CR1: включаем передатчик, приёмник и сам USART */
    MY_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* 4. Очереди и состояние */
    ring_init(&s_txq, s_tx_storage, sizeof(s_tx_storage));
    ring_init(&s_rxq, s_rx_storage, sizeof(s_rx_storage));
    s_mode    = MY_UART_MODE_POLL;
    s_tx_busy = 0;
}

void my_uart_set_mode(my_uart_mode_t mode)
{
    /* Глушим все прерывания USART6 на уровне NVIC */
    nvic_disable_irq(MY_UART_IRQn);

    /* Снимаем разрешения прерываний в самом USART */
    MY_UART->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TXEIE | USART_CR1_TCIE);

    /* Чистим очереди и флаги */
    ring_clear(&s_txq);
    ring_clear(&s_rxq);
    s_tx_busy = 0;

    s_mode = mode;

    if (mode == MY_UART_MODE_IRQ) {
        MY_UART->CR1 |= USART_CR1_RXNEIE;   /* разрешить приём в прерывании */
        nvic_enable_irq(MY_UART_IRQn);
    }
}

my_uart_mode_t my_uart_get_mode(void)
{
    return s_mode;
}

int my_uart_send_byte(uint8_t b)
{
    if (s_mode == MY_UART_MODE_IRQ) {
        /* Кладём байт в очередь */
        if (!ring_push(&s_txq, b)) return 0;

        /* Если передатчик свободен — запускаем передачу и включаем TXEIE */
        if (!s_tx_busy) {
            s_tx_busy = 1;
            MY_UART->CR1 |= USART_CR1_TXEIE;
        }
        return 1;
    } else {
        /* POLL: ждём освобождения TDR с коротким таймаутом (5 мс) */
        uint32_t start = HAL_GetTick();
        while (!(MY_UART->SR & USART_SR_TXE)) {
            if ((HAL_GetTick() - start) > 5u) return 0;
        }
        MY_UART->DR = b;
        return 1;
    }
}

int my_uart_send_str(const char *s)
{
    int n = 0;
    while (*s) {
        if (!my_uart_send_byte((uint8_t)*s)) break;
        s++;
        n++;
    }
    return n;
}

int my_uart_recv_byte(uint8_t *b)
{
    if (s_mode == MY_UART_MODE_IRQ) {
        return ring_pop(&s_rxq, b);
    } else {
        /* POLL: проверяем RXNE один раз, не ждём */
        if (MY_UART->SR & USART_SR_RXNE) {
            *b = (uint8_t)(MY_UART->DR & 0xFFu);
            return 1;
        }
        return 0;
    }
}

/* ============================================================
   IRQ handler (вызывается из USART6_IRQHandler)
   ============================================================ */

void my_uart_irq_handler(void)
{
    uint32_t sr = MY_UART->SR;

    /* --- Приём --- */
    if (sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)(MY_UART->DR & 0xFFu);
        ring_push(&s_rxq, b);
    }

    /* --- Передача --- */
    if ((sr & USART_SR_TXE) && (MY_UART->CR1 & USART_CR1_TXEIE)) {
        uint8_t b;
        if (ring_pop(&s_txq, &b)) {
            MY_UART->DR = b;
        } else {
            /* Больше нечего отправлять — выключаем TXEIE */
            MY_UART->CR1 &= ~USART_CR1_TXEIE;
            s_tx_busy = 0;
        }
    }

    /* --- Сброс флагов ошибок (читаем DR, если были ошибки) --- */
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        (void)MY_UART->DR;
    }
}
