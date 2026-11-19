/* controller.h: termostaatin päätöslogiikka (ydin, DESIGN.md 4.3).
 *
 * decide() saa yhden tapahtuman kerrallaan ja palauttaa, mitä kuoren pitää tehdä.
 * Se ei koske rautaan eikä kysy aikaa itse: aika tulee parametrina, jotta testi voi
 * syöttää minkä tahansa ajan. Puhdasta C:tä, testataan PC:llä.
 *
 * Vaihe 5: lukema tarkistetaan ensin (plausibility.h, F8), sitten lämmitin ohjataan
 * hystereesillä (hysteresis.h). Anturivika ja mahdoton lukema sammuttavat lämmittimen (F7).
 * Hälytys (alarm.h, F3) saa jokaisesta tapahtumasta tiedon: alueella, ulkona vai vika.
 * Jokainen tapahtuma kirjataan historiaan (history.h, F5), myös hylätyt ja viat.
 *
 * Vaihe 8: hiljainen anturi (DESIGN.md 4.5). Jos sensor-threadista ei ole kuulunut mitään
 * silence_ms:ään, se tulkitaan anturiviaksi. Tarkistus tehdään JOKAISELLA tapahtumalla, joka ei
 * tule sensorilta (EV_TICK, nappi, shell), jotta muut tapahtumat eivät peitä hiljaisuutta.
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "alarm.h"
#include "event.h"
#include "history.h"

/* Muistivihko: mitä vastaus riippuu, mutta mikä ei ole tapahtumassa.
 *
 * Kahdenlaista tietoa:
 *   ASETUKSET: kuori asettaa alussa (myöhemmin shell viestillä). decide() vain lukee.
 *   MUISTI:    decide() itse päivittää joka kutsulla. */
struct state {
    /* asetukset */
    int32_t target_centi;   /* tavoite sadasosa-asteina, sama yksikkö kuin event.centi */
    int32_t band_centi;     /* hystereesikaistan leveys, esim. 30 → tavoite ± 0.15 °C */
    int32_t alarm_range_centi;  /* sallittu alue: tavoite ± tämä (F3), esim. 200 = ± 2 °C */
    uint32_t alarm_delay_ms;    /* kuinka kauan poikkeama saa kestää ennen hälytystä (F3) */
    uint32_t silence_ms;        /* sensorin hiljaisuus, jonka jälkeen anturi on vikaantunut (4.5) */

    /* muisti */
    bool heater_on;         /* lämmittimen tila edellisen päätöksen jälkeen */
    bool have_prev;         /* onko edellistä lukemaa (ensimmäisellä lukemalla ei ole) */
    int32_t prev_centi;     /* edellinen LUKEMA, myös hylätty (plausibility.h) */
    struct alarm alarm;     /* hälytyksen tila (alarm.h) */
    struct history history; /* viimeisimmät lukemat shellille (history.h, F5) */
    uint32_t last_heard_ms; /* milloin sensor viimeksi lähetti jotain (lukema tai vika).
                             * Alussa 0 = käynnistyshetki: jos sensor ei koskaan ala, se huomataan. */
};

/* Vastauslappu kuorelle. */
struct actions {
    bool heater_on;         /* kuori asettaa lämmitin-LEDin tämän mukaan */
    bool alarm_on;          /* kuori vilkuttaa hälytys-LEDiä, kun tämä on päällä */
};

struct actions decide(struct state *s, struct event ev, uint32_t now_ms);

#endif /* CONTROLLER_H */
