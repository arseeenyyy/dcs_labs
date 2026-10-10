#include "ring.h"

void ring_init(ring_t *r, uint8_t *storage, uint16_t size)
{
    r->buf  = storage;
    r->size = size;
    r->head = 0;
    r->tail = 0;
}

void ring_clear(ring_t *r)
{
    r->head = 0;
    r->tail = 0;
}

uint16_t ring_count(const ring_t *r)
{
    return (uint16_t)((r->head + r->size - r->tail) % r->size);
}

uint16_t ring_free(const ring_t *r)
{
    return (uint16_t)(r->size - 1u - ring_count(r));
}

int ring_push(ring_t *r, uint8_t b)
{
    uint16_t next = (uint16_t)((r->head + 1u) % r->size);
    if (next == r->tail) return 0;
    r->buf[r->head] = b;
    r->head = next;
    return 1;
}

int ring_pop(ring_t *r, uint8_t *b)
{
    if (r->head == r->tail) return 0;
    *b = r->buf[r->tail];
    r->tail = (uint16_t)((r->tail + 1u) % r->size);
    return 1;
}

int ring_is_empty(const ring_t *r) { return r->head == r->tail; }
int ring_is_full (const ring_t *r) { return ((r->head + 1u) % r->size) == r->tail; }
