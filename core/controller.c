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

/* Onko sensor ollut liian kauan hiljaa? Vähennyslasku uint32_t:nä toimii myös, kun
 * millisekuntilaskuri pyörähtää ympäri (noin 49 vrk välein), sama keino kuin alarm.c:ssä. */
static bool sensor_silent(const struct state *s, uint32_t now_ms)
{
    return (uint32_t)(now_ms - s->last_heard_ms) > s->silence_ms;
}

/* Anturivian seuraukset: sama kuin EV_SENSOR_FAULT (F7), mutta kutsutaan myös hiljaisuudesta. */
static void sensor_fault(struct state *s, uint32_t now_ms)
{
    s->heater_on = false;
    history_add(&s->history, (struct history_entry){
        .t_ms = now_ms, .centi = 0, .kind = HIST_FAULT });
    alarm_update(&s->alarm, ALARM_IN_FAULT, now_ms, s->alarm_delay_ms);
}

struct actions decide(struct state *s, struct event ev, uint32_t now_ms)
{
    /* Vaihe 8: hiljaisuus tarkistetaan ENSIN kaikille tapahtumille, jotka eivät tule sensorilta.
     * Jos tarkistus tehtäisiin vain EV_TICKissä, napin painallukset ja shell-komennot voisivat
     * herättää controlin juuri ennen aikakatkaisua, eikä tick tulisi koskaan (DESIGN.md 4.5). */
    if (ev.type != EV_SAMPLE && ev.type != EV_SENSOR_FAULT && sensor_silent(s, now_ms)) {
        sensor_fault(s, now_ms);
    }

    struct actions act = { .heater_on = false, .alarm_on = false };

    /* Mitä hälytykselle kerrotaan. Oletus on vika: jos jokin haara unohtaa asettaa
     * tämän, tuloksena on turha hälytys eikä hiljaa ohitettu vika. */
    enum alarm_input ain = ALARM_IN_FAULT;

    switch (ev.type) {
    case EV_SAMPLE: {
        /* Uskottavuus ensin: hystereesi ei saa toimia roskan perusteella. */
        s->last_heard_ms = now_ms;
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
        s->last_heard_ms = now_ms;  /* sensor on hengissä, anturi vain ei vastaa */
        s->heater_on = false;       /* F7 */
        ain = ALARM_IN_FAULT;
        history_add(&s->history, (struct history_entry){
            .t_ms = now_ms, .centi = 0, .kind = HIST_FAULT });
        break;

    case EV_BUTTON:
        /* Vaihe 6. Täytä aukko: kumpi funktio kuittaa, alarm_update vai alarm_ack? (alarm.h) */
        alarm_ack(&s->alarm, now_ms, s->alarm_delay_ms);

        /* K5: Tässä on nyt break, joten suoritus jatkuu alas alarm_update()-kutsuun.
         *     Aja testit. Yksi napin testeistä kaatuu: miksi? Vihje: mikä on ain:n arvo,
         *     kun mikään haara ei aseta sitä? Ja tuoko napin painallus uutta tietoa lämpötilasta?
         *     Korjaus: vaihda break return-lauseeseen, joka palauttaa vastauksen muistista
         *     ilman alarm_update()-kutsua, samaan tapaan kuin funktion lopussa:
         *         return (struct actions){ .heater_on = s->heater_on, .alarm_on = s->alarm.on };
         *     Vastaus: ain on oletuksena ALARM_IN_FAULT, joten alarm_update() luuli napin
         *     painallusta anturiviaksi ja hälytti. Nappi ei tuo tietoa lämpötilasta, joten
         *     hälytykselle ei kerrota mitään: palataan suoraan muistista.
         */
        return (struct actions){ .heater_on = s->heater_on, .alarm_on = s->alarm.on };

    case EV_TICK:
        /* Ei uutta tietoa: hiljaisuus tarkistettiin jo funktion alussa. */
        return (struct actions){ .heater_on = s->heater_on, .alarm_on = s->alarm.on };

    case EV_SET_TARGET:
        /* Vaihe 7. Täytä aukko: mikä kenttä tapahtumasta on uusi tavoite? (event.h) */
        s->target_centi = ev.centi;

        /* K6: Lämmitin ja hälytys päivittyvät vasta seuraavassa lukemassa (alle 1 s päästä),
         *     eivät heti. Miksi tässä ei kutsuta hystereesiä edellisellä lukemalla
         *     (s->prev_centi)? Vihje: katso plausibility.h: mitä prev_centi voi sisältää?
         *     Vastaus: prev_centi voi olla mahdoton lukema eli tyhjä, jolloin hystereesi ei saa sitä käsitellä. */

        return (struct actions){ .heater_on = s->heater_on, .alarm_on = s->alarm.on };
    }

    alarm_update(&s->alarm, ain, now_ms, s->alarm_delay_ms);

    act.heater_on = s->heater_on;   /* vastaus = päivitetty muisti */
    act.alarm_on  = s->alarm.on;    /* sama periaate kuin lämmittimellä: vastaus muistista */
    return act;
}
