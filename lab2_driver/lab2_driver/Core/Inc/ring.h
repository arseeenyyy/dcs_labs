#ifndef RING_H
#define RING_H

#include <stdint.h>

typedef struct {
    uint8_t          *buf;
    uint16_t          size;
    volatile uint16_t head;
    volatile uint16_t tail;
} ring_t;

void     ring_init(ring_t *r, uint8_t *storage, uint16_t size);
void     ring_clear(ring_t *r);
uint16_t ring_count(const ring_t *r);
uint16_t ring_free(const ring_t *r);
int      ring_push(ring_t *r, uint8_t b);
int      ring_pop(ring_t *r, uint8_t *b);
int      ring_is_empty(const ring_t *r);
int      ring_is_full(const ring_t *r);

#endif
