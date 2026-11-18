/* test_controller.c: decide()-tyngän testit (vaihe 4).
 *
 * Tynkäsääntö: lämmitin päällä, jos lämpötila < tavoite. Vaiheessa 5 tynkä vaihtuu
 * oikeaan logiikkaan, ja nämä testit päivitetään sen mukaan.
 *
 * Täytä aukot ____ arvolla true tai false. Mieti jokaisen kohdalla ensin itse,
 * mitä sääntö sanoo, älä katso koodia.
 */
#include <stdbool.h>

#include "unity.h"
#include "controller.h"

#define TARGET 2270   /* 22.70 °C */

static struct state s;

void setUp(void)
{
    /* Ajetaan ennen jokaista testiä: jokainen testi alkaa samasta tilasta. */
    s.target_centi = TARGET;
}

void tearDown(void) {}

/* Apufunktio: näyte, jossa on annettu lämpötila. */
static struct event sample(int32_t centi)
{
    struct event ev = { .type = EV_SAMPLE, .centi = centi };
    return ev;
}

static void test_cold_turns_heater_on(void)
{
    struct actions act = decide(&s, sample(2265), 0);   /* 22.65 °C */
    TEST_ASSERT_EQUAL(true, act.heater_on);
}

static void test_warm_turns_heater_off(void)
{
    struct actions act = decide(&s, sample(2275), 0);   /* 22.75 °C */
    TEST_ASSERT_EQUAL(false, act.heater_on);
}

/* Rajatapaus: lämpötila on TÄSMÄLLEEN tavoite. Juuri rajoilla virheet asuvat:
 * < ja <= eroavat vain tässä kohdassa. */
static void test_exactly_target_heater_off(void)
{
    struct actions act = decide(&s, sample(TARGET), 0);
    TEST_ASSERT_EQUAL(false, act.heater_on);
}

/* F7: anturivika sammuttaa lämmittimen, vaikka viimeisin näyte oli kylmä. */
static void test_sensor_fault_turns_heater_off(void)
{
    decide(&s, sample(1000), 0);                         /* 10.00 °C: lämmitin päälle */
    struct event fault = { .type = EV_SENSOR_FAULT, .centi = 0 };
    struct actions act = decide(&s, fault, 1000);
    TEST_ASSERT_EQUAL(false, act.heater_on);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cold_turns_heater_on);
    RUN_TEST(test_warm_turns_heater_off);
    RUN_TEST(test_exactly_target_heater_off);
    RUN_TEST(test_sensor_fault_turns_heater_off);
    return UNITY_END();
}
