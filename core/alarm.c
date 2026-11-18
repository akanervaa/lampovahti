/* alarm.c: lämpötilahälytys. Säännöt ja perustelut: alarm.h.
 *
 * TDD-vaihe VIHREÄ: täytä aukot ____ niin, että kaikki 8 ketjua menevät läpi.
 * Apufunktiot reached() ja snooze_over() on annettu valmiina.
 */
#include "alarm.h"

/* Onko hetki t saavutettu? Erotus luetaan etumerkillisenä, joten vertailu toimii myös,
 * kun millisekuntilaskuri pyörähtää ympäri. */
static bool reached(uint32_t now_ms, uint32_t t_ms)
{
    return (int32_t)(now_ms - t_ms) >= 0;
}

/* Onko torkku ohi (tai ei ollut päällä)? Purkaa päättyneen torkun. */
static bool snooze_over(struct alarm *a, uint32_t now_ms)
{
    if (a->snoozed && reached(now_ms, a->snooze_until_ms)) {
        a->snoozed = false;
    }
    return !a->snoozed;
}

void alarm_update(struct alarm *a, enum alarm_input in, uint32_t now_ms, uint32_t delay_ms)
{
    switch (in) {
    case ALARM_IN_OK:
        a->counting = false;            /* paluu alueelle nollaa laskennan; hälytys JÄÄ */
        break;

    case ALARM_IN_OUT:
        if (!a->counting) {
            a->counting = true;         /* poikkeama alkoi nyt */
            a->since_ms = now_ms;
        }
        /* Kesto = nyt - alku. Raja: tasan 30 000 ms hälyttää (testi 1). */
        if ((uint32_t)(now_ms - a->since_ms) >= delay_ms && snooze_over(a, now_ms)) {
            a->on = true;
        }
        break;

    case ALARM_IN_FAULT:
        if (snooze_over(a, now_ms)) {
            a->on = true;               /* ei viivettä: laite on sokea */
        }
        break;
    }
}

void alarm_ack(struct alarm *a, uint32_t now_ms, uint32_t delay_ms)
{
    if (a->on == false) {
        return;                         /* ei hälytystä: ei torkkua tulevalle viallekaan */
    }
    a->on = false;
    a->snoozed = true;
    a->snooze_until_ms = now_ms + delay_ms;          /* hälytys voi palata vasta tästä hetkestä */
}
