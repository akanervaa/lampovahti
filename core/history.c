/* history.c: rengaspuskuri. Rajapinta ja perustelut: history.h. */
#include "history.h"

void history_add(struct history *h, struct history_entry e)
{
    h->buf[h->head] = e;
    h->head = (h->head + 1) % HISTORY_LEN;     /* lopusta takaisin alkuun */
    if (h->count < HISTORY_LEN) {
        h->count++;                            /* täynnä: count pysyy, vanhin jää alle */
    }
}

size_t history_count(const struct history *h)
{
    return h->count;
}

struct history_entry history_get(const struct history *h, size_t i)
{
    /* Vanhin on count askelta head:n takana. + HISTORY_LEN pitää luvun positiivisena,
     * koska size_t ei voi olla negatiivinen (head - count menisi ympäri). */
    size_t oldest = (h->head + HISTORY_LEN - h->count) % HISTORY_LEN;
    return h->buf[(oldest + i) % HISTORY_LEN];
}
