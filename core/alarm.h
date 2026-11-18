/* alarm.h: lämpötilahälytys ja sen kuittaus (F3, F4, F7). Ydinmoduuli.
 *
 * Säännöt (päätetty vaiheessa 5):
 *   - Poikkeama (lämpötila sallitun alueen ulkopuolella) hälyttää, kun se on kestänyt
 *     YHTÄJAKSOISESTI viiveen verran. Paluu alueelle nollaa laskennan.
 *   - Anturivika hälyttää HETI: laite on sokea, joten "lyhyttä harmitonta" vikaa ei ole.
 *   - Hälytys JÄÄ PÄÄLLE, vaikka lämpötila palaisi alueelle: joku saa tietää, että yöllä
 *     tapahtui jotain. Vain kuittaus sammuttaa sen.
 *   - Kuittaus on torkku: jos vika jatkuu, hälytys palaa viiveen kuluttua kuittauksesta.
 *     Kuittaus ilman päällä olevaa hälytystä ei tee mitään.
 *
 * Tunnettu heikkous: lämpötila, joka heiluu alueen rajalla (29 s ulkona, 1 s sisällä),
 * ei hälytä koskaan, koska jokainen paluu nollaa laskennan.
 *
 * Aika tulee parametrina (now_ms), ja vertailut ovat wrap-turvallisia: uint32_t-millisekunnit
 * pyörähtävät ympäri noin 49 päivän välein.
 */
#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>
#include <stdint.h>

/* Mitä tapahtumasta tiedetään hälytyksen kannalta. decide() päättää tämän. */
enum alarm_input {
    ALARM_IN_OK,        /* lämpötila sallitulla alueella */
    ALARM_IN_OUT,       /* lämpötila sallitun alueen ulkopuolella */
    ALARM_IN_FAULT,     /* anturivika tai mahdoton lukema */
};

struct alarm {
    bool     on;                /* hälytys päällä (LED vilkkuu) */
    bool     counting;          /* poikkeama käynnissä: since_ms on voimassa */
    uint32_t since_ms;          /* milloin nykyinen poikkeama alkoi */
    bool     snoozed;           /* kuitattu: snooze_until_ms on voimassa */
    uint32_t snooze_until_ms;   /* ennen tätä hetkeä hälytys ei palaa */
};

void alarm_update(struct alarm *a, enum alarm_input in, uint32_t now_ms, uint32_t delay_ms);
void alarm_ack(struct alarm *a, uint32_t now_ms, uint32_t delay_ms);

#endif /* ALARM_H */
