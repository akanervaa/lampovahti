/* test_plausibility.c: uskottavuustarkistuksen testit (vaihe 5, TDD).
 *
 * Säännöt (plausibility.h):
 *   alue  -4000 ... 8500, rajat itse sallittuja
 *   hyppy enintään 200 edellisestä lukemasta (kumpaankin suuntaan), jos edellinen on
 *
 * Täytä odotusarvot ____ (true = uskottava, false = hylätään). Päättele SÄÄNNÖSTÄ.
 */
#include <stdbool.h>
#include <stddef.h>

#include "unity.h"
#include "plausibility.h"

struct plaus_case {
    const char *name;
    int32_t     centi;
    bool        have_prev;
    int32_t     prev;
    bool        expect_ok;
};

static const struct plaus_case cases[] = {
    /* Alue, ei edellistä lukemaa */
    { "normaali, ensimmainen",        2270,  false, 0,    true },
    { "tasan alaraja",               -4000,  false, 0,    true },
    { "alarajan alla",               -4001,  false, 0,    false },
    { "tasan ylaraja",                8500,  false, 0,    true},
    { "ylarajan yli",                 8501,  false, 0,    false },
    { "bittivirhe +511.98 (vaihe 4)", 51198, false, 0,    false },

    /* Hyppy edellisestä (2270) */
    { "pieni muutos",                 2285,  true,  2270, true },
    { "ylos tasan 200",               2470,  true,  2270, true },
    { "ylos 201",                     2471,  true,  2270, false},
    { "alas tasan 200",               2070,  true,  2270, true },
    { "alas 201",                     2069,  true,  2270, false },
};

void setUp(void) {}
void tearDown(void) {}

static void test_all_cases(void)
{
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const struct plaus_case *c = &cases[i];
        bool got = plaus_ok(c->centi, c->have_prev, c->prev);
        TEST_ASSERT_EQUAL_MESSAGE(c->expect_ok, got, c->name);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_all_cases);
    return UNITY_END();
}
