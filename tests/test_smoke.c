/* test_smoke.c: putkitesti.
 *
 * Tämä ei testaa projektin koodia. Se todistaa, että testiputki toimii: PC:n gcc kääntää,
 * Unity linkittyy, testi ajetaan ja tulos näkyy. Kun putki on todistettu, oikeiden testien
 * vika on aina testissä tai koodissa, ei ympäristössä (poissulkeminen).
 */
#include "unity.h"

/* Unity kutsuu näitä jokaisen testin ympärillä: setUp ennen, tearDown jälkeen.
 * Niihin laitetaan esim. tilan nollaus, jotta testit eivät vaikuta toisiinsa.
 * Tämä testi ei tarvitse niitä, mutta Unity vaatii, että ne ovat olemassa. */
void setUp(void) {}
void tearDown(void) {}

/* Yksi testi = yksi funktio, joka tarkistaa yhden asian. */
static void test_math_works(void)
{
    /* (1) Tarkista, että 2 + 2 on 4.
     *     Makro: TEST_ASSERT_EQUAL_INT(odotettu, saatu)
     *     Järjestyksellä on väliä: kun testi kaatuu, Unity kirjoittaa
     *     "Expected <odotettu> Was <saatu>". Väärin päin ilmoitus on harhaanjohtava. */
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main(void)
{
    UNITY_BEGIN();
    /* (2) Rekisteröi testi ajettavaksi. Makro: RUN_TEST(testifunktion_nimi) */
    RUN_TEST(test_math_works);
    return UNITY_END();
}
