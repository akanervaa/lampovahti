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

static struct state s;

void setUp(void)
{
    /* Ajetaan ennen jokaista testiä: jokainen testi alkaa samasta tilasta. */
    s = (struct state){ .target_centi = TARGET, .band_centi = BAND, .heater_on = false };
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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hysteresis_remembers_direction);
    RUN_TEST(test_sensor_fault_turns_heater_off);
    RUN_TEST(test_after_fault_heater_stays_off_in_band);
    RUN_TEST(test_glitch_turns_heater_off_then_recovers);
    RUN_TEST(test_real_jump_accepted_after_one_reading);
    return UNITY_END();
}
