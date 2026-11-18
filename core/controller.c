/* controller.c: termostaatin päätöslogiikka. Rajapinta ja perustelut: controller.h.
 *
 * Oletusvastaus on "lämmitin pois" (F7: jumiin päälle jäänyt lämmitin on pahin vika).
 * Switchissä ei ole default-haaraa: kun event.h:hon lisätään uusi tyyppi, kääntäjä
 * (-Wswitch) näyttää tämän kohdan.
 *
 * Vika kirjoitetaan MUISTIIN (s->heater_on), ei vain vastaukseen: muuten muisti sanoisi
 * yhä "päällä", ja kun anturi palaa kaistan sisältä, hystereesi sytyttäisi lämmittimen
 * itsestään. Vastaus otetaan aina muistista, joten ne eivät voi olla eri mieltä.
 */
#include "controller.h"
#include "hysteresis.h"
#include "plausibility.h"

struct actions decide(struct state *s, struct event ev, uint32_t now_ms)
{
    struct actions act = { .heater_on = false, .alarm_on = false };

    /* Mitä hälytykselle kerrotaan. Oletus on vika: jos jokin haara unohtaa asettaa
     * tämän, tuloksena on turha hälytys eikä hiljaa ohitettu vika. */
    enum alarm_input ain = ALARM_IN_FAULT;

    switch (ev.type) {
    case EV_SAMPLE: {
        /* Uskottavuus ensin: hystereesi ei saa toimia roskan perusteella. */
        bool ok = plaus_ok(ev.centi, s->have_prev, s->prev_centi);

        history_add(&s->history, (struct history_entry){
            .t_ms = now_ms, .centi = ev.centi, .kind = ok ? HIST_OK : HIST_REJECTED });

        /* Edellinen lukema päivitetään AINA, myös hylätyllä (plausibility.h). */
        s->prev_centi = ev.centi;
        s->have_prev  = true;

        if (ok) {
            s->heater_on = hysteresis_heater(s->heater_on, ev.centi,
                                             s->target_centi, s->band_centi);

            /* Sallittu alue: tavoite ± alarm_range_centi, rajat mukaan lukien. */
            int32_t dev = ev.centi - s->target_centi;
            if (dev < 0) {
                dev = -dev;
            }
            ain = (dev <= s->alarm_range_centi) ? ALARM_IN_OK : ALARM_IN_OUT;
        } else {
            s->heater_on = false;   /* F8: mahdoton lukema käsitellään kuin anturivika */
            ain = ALARM_IN_FAULT;
        }
        break;
    }

    case EV_SENSOR_FAULT:
        s->heater_on = false;       /* F7 */
        ain = ALARM_IN_FAULT;
        history_add(&s->history, (struct history_entry){
            .t_ms = now_ms, .centi = 0, .kind = HIST_FAULT });
        break;
    }

    alarm_update(&s->alarm, ain, now_ms, s->alarm_delay_ms);

    act.heater_on = s->heater_on;   /* vastaus = päivitetty muisti */
    act.alarm_on  = s->alarm.on;    /* sama periaate kuin lämmittimellä: vastaus muistista */
    return act;
}
