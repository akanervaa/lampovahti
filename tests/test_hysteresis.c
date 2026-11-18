/* test_hysteresis.c: hystereesin testit (vaihe 5, TDD).
 *
 * Tavoite 2270 (22.70 °C), kaista 30 → alaraja 2255, yläraja 2285.
 * Sääntö: < 2255 → päälle, > 2285 → pois, muuten (myös tasan rajalla) entinen tila.
 *
 * Täytä odotusarvot ____ (true = päällä, false = pois). Päättele SÄÄNNÖSTÄ, ei koodista.
 *
 * Taulukkotesti: jokainen rivi on yksi tapaus. Uuden tapauksen lisääminen on yksi rivi,
 * ja kaatuessa viesti kertoo, mikä rivi kaatui.
 */
#include <stdbool.h>
#include <stdio.h>

#include "unity.h"
#include "hysteresis.h"

#define TARGET 2270
#define BAND   30

struct hyst_case {
    const char *name;
    bool        was_on;     /* lämmittimen tila ennen kutsua */
    int32_t     temp;
    bool        expect_on;
};

static const struct hyst_case cases[] = {
    /* Kaikki 6 yhdistelmää: (alle / kaistalla / yli) × (oli pois / oli päällä) */
    { "kylma, oli pois",          false, 2200, true },
    { "kylma, oli paalla",        true,  2200, true },
    { "kaistalla, oli pois",      false, 2270, false },
    { "kaistalla, oli paalla",    true,  2270, true },
    { "kuuma, oli pois",          false, 2300, false },
    { "kuuma, oli paalla",        true,  2300, false },

    /* Rajatapaukset: tasan rajalla ja yksi sadasosa ohi */
    { "tasan alarajalla, pois",   false, 2255, false},
    { "alarajan alla, pois",      false, 2254, true },
    { "tasan ylarajalla, paalla", true,  2285, true },
    { "ylarajan yli, paalla",     true,  2286, false },
};

void setUp(void) {}
void tearDown(void) {}

static void test_all_cases(void)
{
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const struct hyst_case *c = &cases[i];
        bool got = hysteresis_heater(c->was_on, c->temp, TARGET, BAND);
        TEST_ASSERT_EQUAL_MESSAGE(c->expect_on, got, c->name);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_all_cases);
    return UNITY_END();
}
