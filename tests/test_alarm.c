/* test_alarm.c: hälytyksen testit (vaihe 5, TDD).
 *
 * Viive 30 000 ms. Jokainen testi on TAPAHTUMAKETJU: rivi = (mitä tapahtui, milloin,
 * onko hälytys päällä rivin JÄLKEEN). Säännöt: alarm.h.
 *
 * Täytä odotusarvot ____ (true = hälytys päällä). Käy ketju läpi rivi kerrallaan ja pidä
 * kirjaa kolmesta asiasta: milloin nykyinen poikkeama alkoi, onko torkku päällä ja mihin
 * asti, ja onko hälytys päällä.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "unity.h"
#include "alarm.h"

#define DELAY 30000u

enum step_kind { OK, OUT, FAULT, ACK };

struct step {
    enum step_kind kind;
    uint32_t       at_ms;
    bool           expect_on;
};

/* Ajaa ketjun alusta (hälytys pois) ja tarkistaa jokaisen rivin. */
static void run(const struct step *steps, size_t n)
{
    struct alarm a = { 0 };
    for (size_t i = 0; i < n; i++) {
        const struct step *s = &steps[i];
        switch (s->kind) {
        case OK:    alarm_update(&a, ALARM_IN_OK,    s->at_ms, DELAY); break;
        case OUT:   alarm_update(&a, ALARM_IN_OUT,   s->at_ms, DELAY); break;
        case FAULT: alarm_update(&a, ALARM_IN_FAULT, s->at_ms, DELAY); break;
        case ACK:   alarm_ack(&a, s->at_ms, DELAY);                    break;
        }
        char msg[48];
        snprintf(msg, sizeof msg, "rivi %u (t = %lu ms)", (unsigned)(i + 1),
                 (unsigned long)s->at_ms);
        TEST_ASSERT_EQUAL_MESSAGE(s->expect_on, a.on, msg);
    }
}
#define RUN_CHAIN(steps) run(steps, sizeof(steps) / sizeof(steps[0]))

void setUp(void) {}
void tearDown(void) {}

/* Raja: tasan 30 s poikkeamaa hälyttää, 1 ms vähemmän ei. */
static void test_long_deviation_alarms(void)
{
    static const struct step s[] = {
        { OUT,  0,     false },
        { OUT,  29999, false },
        { OUT,  30000, true  },
    };
    RUN_CHAIN(s);
}

/* Lyhyt poikkeama ja paluu: laskenta alkaa alusta. */
static void test_return_restarts_count(void)
{
    static const struct step s[] = {
        { OUT,  0,     false },
        { OUT,  20000, false },
        { OK,   21000, false },    /* paluu alueelle */
        { OUT,  40000, false },    /* uusi poikkeama alkaa */
        { OUT,  60000, false },     /* 60 s alusta, mutta kuinka kauan TÄTÄ poikkeamaa? */
        { OUT,  70000, true },
    };
    RUN_CHAIN(s);
}

/* Hälytys jää päälle, kun lämpötila palaa. Kuittaus sammuttaa. */
static void test_alarm_latches_until_ack(void)
{
    static const struct step s[] = {
        { OUT,  0,     false },
        { OUT,  30000, true  },
        { OK,   31000, true },     /* lämpötila palasi itsestään */
        { ACK,  32000, false },
        { OK,   33000, false },
    };
    RUN_CHAIN(s);
}

/* Kuittaus kesken poikkeaman on torkku: hälytys palaa 30 s kuittauksesta. */
static void test_ack_snoozes(void)
{
    static const struct step s[] = {
        { OUT,  0,     false },
        { OUT,  30000, true  },
        { ACK,  31000, false },
        { OUT,  50000, false },     /* poikkeama on kestänyt 50 s, mutta torkku? */
        { OUT,  61000, true },
    };
    RUN_CHAIN(s);
}

/* Anturivika hälyttää heti. Kuittaus torkuttaa senkin. */
static void test_fault_alarms_immediately(void)
{
    static const struct step s[] = {
        { FAULT, 0,     true  },
        { ACK,   1000,  false },
        { FAULT, 2000,  false  },
        { FAULT, 31000, true  },
    };
    RUN_CHAIN(s);
}

/* Kesken poikkeaman tuleva vika ei odota poikkeaman 30 sekuntia. */
static void test_fault_during_deviation_alarms_immediately(void)
{
    static const struct step s[] = {
        { OUT,   0,    false },
        { FAULT, 5000, true},
    };
    RUN_CHAIN(s);
}

/* Kuittaus ilman hälytystä ei saa torkuttaa tulevaa vikaa. */
static void test_ack_without_alarm_does_nothing(void)
{
    static const struct step s[] = {
        { ACK,   0,    false },
        { FAULT, 1000, true },
    };
    RUN_CHAIN(s);
}

/* Laskuri pyörähtää ympäri (noin 49 päivää käynnissä) kesken poikkeaman.
 * Annettu valmiina: mieti, miksi tämä toimii (alarm.c, now_ms - since_ms). */
static void test_wraparound(void)
{
    static const struct step s[] = {
        { OUT, 0xFFFFF000u, false },    /* 4096 ms ennen pyörähdystä */
        { OUT, 0x00005000u, false },    /* pyörähti: 4096 + 20480 = 24 576 ms poikkeamaa */
        { OUT, 0x00006A30u, true  },    /* 4096 + 27184 = 31 280 ms poikkeamaa */
    };
    RUN_CHAIN(s);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_long_deviation_alarms);
    RUN_TEST(test_return_restarts_count);
    RUN_TEST(test_alarm_latches_until_ack);
    RUN_TEST(test_ack_snoozes);
    RUN_TEST(test_fault_alarms_immediately);
    RUN_TEST(test_fault_during_deviation_alarms_immediately);
    RUN_TEST(test_ack_without_alarm_does_nothing);
    RUN_TEST(test_wraparound);
    return UNITY_END();
}
