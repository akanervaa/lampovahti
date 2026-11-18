/* controller.h: termostaatin päätöslogiikka (ydin, DESIGN.md 4.3).
 *
 * decide() saa yhden tapahtuman kerrallaan ja palauttaa, mitä kuoren pitää tehdä.
 * Se ei koske rautaan eikä kysy aikaa itse: aika tulee parametrina, jotta testi voi
 * syöttää minkä tahansa ajan. Puhdasta C:tä, testataan PC:llä.
 *
 * Vaihe 5: lukema tarkistetaan ensin (plausibility.h, F8), sitten lämmitin ohjataan
 * hystereesillä (hysteresis.h). Anturivika ja mahdoton lukema sammuttavat lämmittimen (F7). Tulossa: uskottavuus, hälytys, historia.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "event.h"

/* Muistivihko: mitä vastaus riippuu, mutta mikä ei ole tapahtumassa.
 *
 * Kahdenlaista tietoa:
 *   ASETUKSET: kuori asettaa alussa (myöhemmin shell viestillä). decide() vain lukee.
 *   MUISTI:    decide() itse päivittää joka kutsulla.
 * Tulossa: aika, jolloin lämpötila lähti sallitulta alueelta (F3). */
struct state {
    /* asetukset */
    int32_t target_centi;   /* tavoite sadasosa-asteina, sama yksikkö kuin event.centi */
    int32_t band_centi;     /* hystereesikaistan leveys, esim. 30 → tavoite ± 0.15 °C */

    /* muisti */
    bool heater_on;         /* lämmittimen tila edellisen päätöksen jälkeen */
    bool have_prev;         /* onko edellistä lukemaa (ensimmäisellä lukemalla ei ole) */
    int32_t prev_centi;     /* edellinen LUKEMA, myös hylätty (plausibility.h) */
};

/* Vastauslappu kuorelle. */
struct actions {
    bool heater_on;         /* kuori asettaa lämmitin-LEDin tämän mukaan */
};

struct actions decide(struct state *s, struct event ev, uint32_t now_ms);

#endif /* CONTROLLER_H */
