/* test_mys1_conv.c: raakaluku → sadasosa-asteet.
 *
 * Testausstrategia (DESIGN.md 4.3): kiinteät esimerkit (luettavia, toimivat dokumentaationa)
 * + KAIKKI 65 536 syötettä verrattuna toiseen laskutapaan.
 *
 * Laske odotetut arvot itse käsin: raw × 100 / 64, ja desimaalit katkaistaan nollaa kohti.
 */
#include <stdio.h>      /* snprintf */
#include "unity.h"
#include "mys1_conv.h"

void setUp(void) {}
void tearDown(void) {}

/* --- Kiinteät esimerkit --------------------------------------------------------------------- */

/* Löytää laskujärjestysvirheen: jos jaetaan ensin (1440 / 64 * 100), tulos on 2200. */
static void test_room_temperature(void)
{
    TEST_ASSERT_EQUAL_INT32(2250, mys1_raw_to_centi(1440));
}

/* Löytää etumerkkivirheen: etumerkittömänä -64 olisi 65472 → +1023 °C. */
static void test_negative(void)
{
    TEST_ASSERT_EQUAL_INT32(-100, mys1_raw_to_centi(-64));        /* (1) -1.00 °C sadasosina */
}

static void test_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(0, mys1_raw_to_centi(0));
}

/* Katkaisu nollaa kohti: tarkistaa pyöristyspäätöksen molempiin suuntiin. */
static void test_truncates_toward_zero(void)
{
    TEST_ASSERT_EQUAL_INT32(1, mys1_raw_to_centi(1));         /* (2)  1.5625 → ? */
    TEST_ASSERT_EQUAL_INT32(-1, mys1_raw_to_centi(-1));        /* (3) -1.5625 → ? */
}

/* Ääripäät: 32767 × 100 ei mahdu 16 bittiin, joten nämä löytävät ylivuodon. */
static void test_extremes(void)
{
    TEST_ASSERT_EQUAL_INT32(51198, mys1_raw_to_centi(32767));     /* (4) 32767 × 100 / 64 = ? */
    TEST_ASSERT_EQUAL_INT32(-51200, mys1_raw_to_centi(-32768));    /* (5) -32768 × 100 / 64 = ? */
}

/* --- Kaikki syötteet ------------------------------------------------------------------------ */

/* Verrataan jokaista mahdollista raakalukua toiseen laskutapaan: liukulukulaskuun.
 * (int32_t)-muunnos liukuluvusta katkaisee nollaa kohti, eli sama sääntö kuin funktiossa. */
static void test_all_inputs_match_reference(void)
{
    for (int32_t raw = INT16_MIN; raw <= INT16_MAX; raw++) {
        int32_t expected = (int32_t)((double)raw * 100.0 / 64.0);
        int32_t actual = mys1_raw_to_centi((int16_t)raw);
        if (expected != actual) {
            /* Kerrotaan ENSIMMÄINEN väärä syöte, jotta vian voi toistaa yhdellä luvulla. */
            char msg[64];
            snprintf(msg, sizeof msg, "raw = %ld", (long)raw);
            TEST_ASSERT_EQUAL_INT32_MESSAGE(expected, actual, msg);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_room_temperature);
    RUN_TEST(test_negative);
    RUN_TEST(test_zero);
    RUN_TEST(test_truncates_toward_zero);
    RUN_TEST(test_extremes);
    RUN_TEST(test_all_inputs_match_reference);
    return UNITY_END();
}
