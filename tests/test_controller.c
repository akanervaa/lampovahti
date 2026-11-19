/* test_controller.c: decide()-funktion testit.
 *
 * Tavoite 2270, kaista 30 → alaraja 2255, yläraja 2285 (hysteresis.h).
 * Yksittäiset hystereesin tapaukset testataan test_hysteresis.c:ssä. Täällä testataan,
 * että decide() MUISTAA tilan kutsujen välillä ja käyttää sitä oikein: siksi testit
 * ovat tapahtumaketjuja eivätkä yksittäisiä kutsuja.
 *
 * Täytä odotusarvot ____ (true/false). Käy ketju läpi rivi kerrallaan ja pidä kirjaa,
 * mikä lämmittimen tila on kunkin rivin JÄLKEEN: se on seuraavan rivin "edellinen tila".
 */
#include <stdbool.h>

#include "unity.h"
#include "controller.h"

#define TARGET 2270
#define BAND   30
#define RANGE  200      /* sallittu alue 2070 ... 2470 */
#define DELAY  30000u
#define SILENCE 2500u   /* sensorin hiljaisuus, jonka jälkeen anturivika (vaihe 8) */

static struct state s;

void setUp(void)
{
    /* Ajetaan ennen jokaista testiä: jokainen testi alkaa samasta tilasta. */
    s = (struct state){ .target_centi = TARGET, .band_centi = BAND,
                        .alarm_range_centi = RANGE, .alarm_delay_ms = DELAY,
                        .silence_ms = SILENCE };
}

void tearDown(void) {}

/* Apufunktiot: tapahtumat ja yksi decide()-kutsu. */
static bool sample(int32_t centi)
{
    struct event ev = { .type = EV_SAMPLE, .centi = centi };
    return decide(&s, ev, 0).heater_on;
}

static bool fault(void)
{
    struct event ev = { .type = EV_SENSOR_FAULT, .centi = 0 };
    return decide(&s, ev, 0).heater_on;
}

/* Hälytystestien apufunktiot: palauttavat hälytyksen tilan, aika annetaan. */
static bool alarm_after_sample(int32_t centi, uint32_t at_ms)
{
    struct event ev = { .type = EV_SAMPLE, .centi = centi };
    return decide(&s, ev, at_ms).alarm_on;
}

static bool alarm_after_fault(uint32_t at_ms)
{
    struct event ev = { .type = EV_SENSOR_FAULT, .centi = 0 };
    return decide(&s, ev, at_ms).alarm_on;
}

static bool alarm_after_button(uint32_t at_ms)
{
    struct event ev = { .type = EV_BUTTON, .centi = 0 };
    return decide(&s, ev, at_ms).alarm_on;
}

static void set_target(int32_t centi)
{
    struct event ev = { .type = EV_SET_TARGET, .centi = centi };
    (void)decide(&s, ev, 0);
}

/* Lämpötila nousee ja laskee kaistan läpi. Kaistan sisällä vastaus riippuu
 * siitä, mistä suunnasta tultiin. */
static void test_hysteresis_remembers_direction(void)
{
    TEST_ASSERT_EQUAL(false, sample(2270));   /* alussa pois, kaistalla */
    TEST_ASSERT_EQUAL(true,  sample(2250));   /* alle alarajan */
    TEST_ASSERT_EQUAL(true,  sample(2270));   /* kaistalla */
    TEST_ASSERT_EQUAL(true,  sample(2285));   /* tasan ylärajalla */
    TEST_ASSERT_EQUAL(false, sample(2290));   /* yli ylärajan */
    TEST_ASSERT_EQUAL(false, sample(2270));   /* kaistalla */
}

/* F7: anturivika sammuttaa lämmittimen. */
static void test_sensor_fault_turns_heater_off(void)
{
    TEST_ASSERT_EQUAL(true,  sample(2200));   /* kylmä: päälle */
    TEST_ASSERT_EQUAL(false, fault());
}

/* Anturi palaa vian jälkeen kaistan sisältä. */
static void test_after_fault_heater_stays_off_in_band(void)
{
    sample(2200);                             /* päälle */
    fault();                                  /* pois */
    TEST_ASSERT_EQUAL(false, sample(2270));
}

/* F8: yksi bittivirhe kesken lämmityksen. */
static void test_glitch_turns_heater_off_then_recovers(void)
{
    TEST_ASSERT_EQUAL(true,  sample(2200));   /* kylmä */
    TEST_ASSERT_EQUAL(false, sample(51198));  /* bittivirhe +511.98 */
    TEST_ASSERT_EQUAL(false, sample(2200));   /* paluu takaisin: ? */
    TEST_ASSERT_EQUAL(true,  sample(2200));   /* ? */
}

/* F8: oikea äkillinen muutos. Hylätään kerran, sitten hyväksytään. */
static void test_real_jump_accepted_after_one_reading(void)
{
    TEST_ASSERT_EQUAL(true,  sample(2000));   /* 20.00, kylmä */
    TEST_ASSERT_EQUAL(false, sample(1500));   /* 15.00: ? */
    TEST_ASSERT_EQUAL(true,  sample(1510));   /* ? */
}

/* F3: decide() kertoo hälytykselle oikean asian. Hälytyksen omat säännöt testataan
 * test_alarm.c:ssä; täällä vain, että kytkentä toimii. */
static void test_alarm_range_edges(void)
{
    TEST_ASSERT_EQUAL(false, alarm_after_sample(2470, 0));      /* tasan +2.00: alueella */
    TEST_ASSERT_EQUAL(false, alarm_after_sample(2470, 30000));  /* alueella, ei hälytä */
}

static void test_alarm_out_of_range_for_delay(void)
{
    TEST_ASSERT_EQUAL(false, alarm_after_sample(2471, 0));      /* +2.01: ulkona */
    TEST_ASSERT_EQUAL(true,  alarm_after_sample(2471, 30000));  /* 30 s ulkona */
}

static void test_sensor_fault_alarms_immediately(void)
{
    TEST_ASSERT_EQUAL(true, alarm_after_fault(0));
}

static void test_implausible_reading_alarms_immediately(void)
{
    TEST_ASSERT_EQUAL(false, alarm_after_sample(2270, 0));
    TEST_ASSERT_EQUAL(true,  alarm_after_sample(51198, 1000)); /* bittivirhe */
}

/* F5: historiaan päätyvät kaikki tapahtumat, myös hylätyt ja viat. */
static void test_history_records_everything(void)
{
    alarm_after_sample(2270, 1000);
    alarm_after_sample(51198, 2000);                /* hylätään */
    alarm_after_fault(3000);

    TEST_ASSERT_EQUAL_size_t(3, history_count(&s.history));
    TEST_ASSERT_EQUAL(HIST_OK,       history_get(&s.history, 0).kind);
    TEST_ASSERT_EQUAL(HIST_REJECTED, history_get(&s.history, 1).kind);
    TEST_ASSERT_EQUAL_INT32(51198,   history_get(&s.history, 1).centi);
    TEST_ASSERT_EQUAL(HIST_FAULT,    history_get(&s.history, 2).kind);
    TEST_ASSERT_EQUAL_UINT32(3000,   history_get(&s.history, 2).t_ms);
}

/* Vaihe 6: nappi. Täytä odotusarvot ____ (true/false). Säännöt: alarm.h (kuittaus = torkku). */

/* Vika hälyttää, nappi kuittaa, vika jatkuu torkun ajan hiljaa, torkun jälkeen hälytys palaa. */
static void test_button_acks_alarm(void)
{
    TEST_ASSERT_TRUE(alarm_after_fault(0));
    TEST_ASSERT_EQUAL(false, alarm_after_button(1000));
    TEST_ASSERT_EQUAL(false, alarm_after_fault(2000));       /* torkku kestää 1000 + 30000 asti */
    TEST_ASSERT_EQUAL(true, alarm_after_fault(31000));
}

/* Kaikki kunnossa, joku painaa nappia uteliaisuuttaan. */
static void test_button_without_alarm_does_nothing(void)
{
    TEST_ASSERT_FALSE(alarm_after_sample(2270, 0));
    TEST_ASSERT_EQUAL(false, alarm_after_button(1000));
}

/* Nappi ei koske lämmittimeen: se kuittaa vain hälytyksen. */
static void test_button_keeps_heater(void)
{
    TEST_ASSERT_TRUE(sample(2200));         /* alle alarajan 2255: lämmitin päälle */
    struct event ev = { .type = EV_BUTTON, .centi = 0 };
    TEST_ASSERT_EQUAL(true, decide(&s, ev, 0).heater_on);
}

/* Vaihe 7: tavoitteen vaihto. Täytä ____ (true/false). Kaista on aina tavoite ± 15. */

/* Sama lämpötila, eri tavoite: kaista siirtyy lämpötilan ylä- tai alapuolelle. */
static void test_set_target_moves_band(void)
{
    TEST_ASSERT_FALSE(sample(2270));            /* tavoite 22.70: kaistan sisällä, pois */
    set_target(2500);                           /* kaista 24.85 ... 25.15 */
    TEST_ASSERT_EQUAL(true, sample(2270));
    set_target(2000);                           /* kaista 19.85 ... 20.15 */
    TEST_ASSERT_EQUAL(false, sample(2270));
}

/* Tavoitteen vaihto ei itse koske lämmittimeen: vasta seuraava lukema ratkaisee (K6). */
static void test_set_target_waits_for_reading(void)
{
    TEST_ASSERT_TRUE(sample(2200));             /* alle 22.55: päälle */
    struct event ev = { .type = EV_SET_TARGET, .centi = 2000 };
    TEST_ASSERT_EQUAL(true, decide(&s, ev, 0).heater_on);
}

/* Vaihe 8: hiljainen anturi (DESIGN.md 4.5). */

static struct actions at(enum event_type type, int32_t centi, uint32_t at_ms)
{
    struct event ev = { .type = type, .centi = centi };
    return decide(&s, ev, at_ms);
}

/* Raja: tasan 2500 ms hiljaisuutta on vielä ok, 2501 ms on vika. */
static void test_silence_edge(void)
{
    TEST_ASSERT_TRUE(at(EV_SAMPLE, 2200, 0).heater_on);
    TEST_ASSERT_TRUE(at(EV_TICK, 0, 2500).heater_on);
    struct actions a = at(EV_TICK, 0, 2501);
    TEST_ASSERT_FALSE(a.heater_on);
    TEST_ASSERT_TRUE(a.alarm_on);               /* vika hälyttää heti */
}

/* DESIGN.md:n esimerkki: napin painallukset eivät peitä hiljaisuutta. */
static void test_buttons_do_not_hide_silence(void)
{
    TEST_ASSERT_TRUE(at(EV_SAMPLE, 2200, 0).heater_on);
    TEST_ASSERT_TRUE(at(EV_BUTTON, 0, 1000).heater_on);
    TEST_ASSERT_TRUE(at(EV_BUTTON, 0, 2000).heater_on);
    TEST_ASSERT_FALSE(at(EV_BUTTON, 0, 3000).heater_on);
}

/* Sensor ilmoittaa anturiviasta: se on hengissä, joten hiljaisuuslaskenta alkaa alusta. */
static void test_fault_report_is_not_silence(void)
{
    (void)at(EV_SENSOR_FAULT, 0, 2000);
    size_t before = history_count(&s.history);
    (void)at(EV_TICK, 0, 4000);                 /* 2000 ms viimeisestä viestistä: ei hiljaisuutta */
    TEST_ASSERT_EQUAL_size_t(before, history_count(&s.history));
}

/* Laskurin pyörähdys: viimeinen lukema juuri ennen ympäripyörähdystä. */
static void test_silence_across_wrap(void)
{
    TEST_ASSERT_TRUE(at(EV_SAMPLE, 2200, 0xFFFFF000u).heater_on);   /* 4096 ms ennen pyörähdystä */
    TEST_ASSERT_TRUE(at(EV_TICK, 0, 0xFFFFF9C4u).heater_on);        /* tasan 2500 ms: ok */
    /* 4096 + 256 = 4352 ms: vika. Suora vertailu now > last + silence luulisi tätä
     * hetkeä MENNEISYYDEKSI (0x100 < 0xFFFFF9C4) eikä huomaisi hiljaisuutta. */
    TEST_ASSERT_FALSE(at(EV_TICK, 0, 0x00000100u).heater_on);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hysteresis_remembers_direction);
    RUN_TEST(test_sensor_fault_turns_heater_off);
    RUN_TEST(test_after_fault_heater_stays_off_in_band);
    RUN_TEST(test_glitch_turns_heater_off_then_recovers);
    RUN_TEST(test_real_jump_accepted_after_one_reading);
    RUN_TEST(test_alarm_range_edges);
    RUN_TEST(test_alarm_out_of_range_for_delay);
    RUN_TEST(test_sensor_fault_alarms_immediately);
    RUN_TEST(test_implausible_reading_alarms_immediately);
    RUN_TEST(test_history_records_everything);
    RUN_TEST(test_button_acks_alarm);
    RUN_TEST(test_button_without_alarm_does_nothing);
    RUN_TEST(test_button_keeps_heater);
    RUN_TEST(test_set_target_moves_band);
    RUN_TEST(test_set_target_waits_for_reading);
    RUN_TEST(test_silence_edge);
    RUN_TEST(test_buttons_do_not_hide_silence);
    RUN_TEST(test_fault_report_is_not_silence);
    RUN_TEST(test_silence_across_wrap);
    return UNITY_END();
}
