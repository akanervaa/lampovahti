/* controller.c: termostaatin päätöslogiikka. Rajapinta ja perustelut: controller.h.
 *
 * Oletusvastaus on "lämmitin pois" (F7: jumiin päälle jäänyt lämmitin on pahin vika).
 * Switchissä ei ole default-haaraa: kun event.h:hon lisätään uusi tyyppi, kääntäjä
 * (-Wswitch) näyttää tämän kohdan.
 *
 * Täytä aukot ____ ja vastaa kysymykseen K1.
 */
#include "controller.h"
#include "hysteresis.h"

struct actions decide(struct state *s, struct event ev, uint32_t now_ms)
{
    struct actions act = { .heater_on = false };

    (void)now_ms;   /* aikaa tarvitaan vasta hälytyksessä (F3) */

    switch (ev.type) {
    case EV_SAMPLE:
        s->heater_on = hysteresis_heater(s->heater_on, ev.centi,
                                         s->target_centi, s->band_centi);
        break;

    case EV_SENSOR_FAULT:
        /* K1: Miksi vika kirjoitetaan MUISTIIN (s->heater_on) eikä pelkästään vastaukseen
         *     (act.heater_on)? Mitä tapahtuisi, kun anturi palaa ja ensimmäinen lukema on
         *     kaistan sisällä? (Testi test_after_fault_heater_stays_off_in_band.)
         *     Vastaus: Koska vika voi olla tilapäinen
         */
        s->heater_on = false;   /* vika: pois */
        break;
    }

    act.heater_on = s->heater_on;       /* vastaus = päivitetty muisti */
    return act;
}
