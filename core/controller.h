/* controller.h: termostaatin päätöslogiikka (ydin, DESIGN.md 4.3).
 *
 * decide() saa yhden tapahtuman kerrallaan ja palauttaa, mitä kuoren pitää tehdä.
 * Se ei koske rautaan eikä kysy aikaa itse: aika tulee parametrina, jotta testi voi
 * syöttää minkä tahansa ajan. Puhdasta C:tä, testataan PC:llä.
 *
 * VAIHE 4: tynkä. Sääntö: lämmitin päällä, jos lämpötila < tavoite. Ei hystereesiä,
 * joten LED räpsyy tavoitteen ympärillä (tarkoituksella, ks. vaihe 5).
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "event.h"

/* Muistivihko: mitä vastaus riippuu, mutta mikä ei ole tapahtumassa.
 * Vaihe 5 lisää tänne mm. heater_on (hystereesi) ja ajan, jolloin lämpötila lähti
 * sallitulta alueelta (F3). */
struct state {
    int32_t target_centi;   /* tavoite sadasosa-asteina, sama yksikkö kuin event.centi */
};

/* Vastauslappu kuorelle. */
struct actions {
    bool heater_on;         /* kuori asettaa lämmitin-LEDin tämän mukaan */
};

struct actions decide(struct state *s, struct event ev, uint32_t now_ms);

#endif /* CONTROLLER_H */
