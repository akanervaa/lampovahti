/* history.h: viimeisimmät lukemat shellin history-komennolle (F5). Ydinmoduuli.
 *
 * Rengaspuskuri: kiinteä taulukko (ei mallocia). Kun se on täynnä, uusin kirjoittaa
 * vanhimman päälle, joten historia näyttää aina viimeiset HISTORY_LEN tapahtumaa.
 *
 * Mukana ovat myös hylätyt lukemat ja anturiviat: muuten historia näyttäisi tasaista
 * lämpötilaa, ja hälytyksen syy (esim. bittivirhe) jäisi näkymättä.
 *
 * Rajoitus: 1 lukema/s → 32 riviä kattaa vain noin puoli minuuttia. Historia näyttää siis
 * "mitä tapahtuu nyt". Hälytyksen syy jälkikäteen löytyy lokista (F6).
 * Myöhempi parannus: oma pieni rengas harvinaisille tapahtumille (hälytys, vika, kuittaus),
 * jotta tiheät lukemat eivät työnnä niitä pois.
 */
#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>
#include <stdint.h>

#define HISTORY_LEN 32

enum history_kind {
    HIST_OK,            /* uskottava lukema */
    HIST_REJECTED,      /* mahdoton lukema (F8): centi on anturin antama arvo */
    HIST_FAULT,         /* anturi ei vastannut (F7): centi ei merkitse mitään */
};

struct history_entry {
    uint32_t          t_ms;
    int32_t           centi;
    enum history_kind kind;
};

struct history {
    struct history_entry buf[HISTORY_LEN];
    size_t head;        /* seuraava kirjoituspaikka */
    size_t count;       /* montako riviä on käytössä (enintään HISTORY_LEN) */
};

void   history_add(struct history *h, struct history_entry e);
size_t history_count(const struct history *h);

/* i = 0 on vanhin, i = history_count() - 1 uusin. */
struct history_entry history_get(const struct history *h, size_t i);

#endif /* HISTORY_H */
