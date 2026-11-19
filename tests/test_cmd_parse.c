/* test_cmd_parse.c: tavoitelämpötilan jäsentäjän testit (vaihe 7, TDD).
 *
 * Kaksi osaa (DESIGN.md: suuri syöteavaruus):
 *   1. Taulukko tunnetuista tapauksista. Täytä aukot ____ SÄÄNNÖISTÄ (cmd_parse.h):
 *      tulos on PARSE_OK, PARSE_EMPTY, PARSE_SYNTAX tai PARSE_RANGE, ja arvo sadasosina
 *      (arvolla on merkitystä vain, kun tulos on PARSE_OK; muuten 0).
 *   2. Satunnaistestit kiinteällä siemenellä (valmiina). Jos ne löytävät bugin, syöte
 *      lisätään taulukkoon kiinteäksi tapaukseksi.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity.h"
#include "cmd_parse.h"

struct parse_case {
    const char       *text;
    enum parse_result expect;
    int32_t           centi;
};

static const struct parse_case cases[] = {
    /* Hyväksytyt muodot */
    { "23.5",        PARSE_OK,     2350 },
    { "23",          PARSE_OK,     2300 },
    { "23.55",       PARSE_OK,     2355 },
    { "23,5",        PARSE_OK,     2350 },
    { "23.05",       PARSE_OK,         2305 },   /* tarkkana: mikä luku tämä on? */

    /* Alueen rajat: 5.00 ... 35.00 */
    { "5",           PARSE_OK,     500  },
    { "4.99",        PARSE_RANGE,         0   },
    { "35",          PARSE_OK,         3500 },
    { "35.01",       PARSE_RANGE,  0    },
    { "-5",          PARSE_RANGE,         0    },
    { "150",         PARSE_RANGE,  0    },
    { "99999999999", PARSE_RANGE,         0    },   /* ei saa pyörähtää ympäri */
    { "4294967319",  PARSE_RANGE,  0    },   /* 2^32 + 23: ilman suojaa pyörähtäisi 23:ksi = 23.00 °C! */

    /* Hylätyt muodot */
    { "",            PARSE_EMPTY,  0    },
    { "abc",         PARSE_SYNTAX, 0    },
    { "23.5abc",     PARSE_SYNTAX, 0    },
    { "23.555",      PARSE_SYNTAX, 0    },
    { "23.",         PARSE_SYNTAX, 0    },
    { ".5",          PARSE_SYNTAX, 0    },
    { "+23",         PARSE_SYNTAX, 0    },
    { "2 3",         PARSE_SYNTAX, 0    },
    { "--5",         PARSE_SYNTAX, 0    },
};

void setUp(void) {}
void tearDown(void) {}

static void test_table(void)
{
    char msg[64];
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        const struct parse_case *c = &cases[i];
        int32_t got = -12345;                       /* merkki: "ei kirjoitettu" */
        enum parse_result r = parse_target_centi(c->text, &got);

        snprintf(msg, sizeof msg, "syote \"%s\"", c->text);
        TEST_ASSERT_EQUAL_INT_MESSAGE(c->expect, r, msg);
        if (c->expect == PARSE_OK) {
            TEST_ASSERT_EQUAL_INT32_MESSAGE(c->centi, got, msg);
        } else {
            TEST_ASSERT_EQUAL_INT32_MESSAGE(-12345, got, msg);   /* virheessä ei kosketa */
        }
    }
}

/* Satunnainen sallittu arvo → teksti (kuten käyttäjä kirjoittaisi) → jäsennys → sama arvo. */
static void test_random_roundtrip(void)
{
    srand(12345);                                   /* kiinteä siemen: toistettava */
    char text[16];
    for (int n = 0; n < 10000; n++) {
        int32_t v = TARGET_MIN_CENTI + rand() % (TARGET_MAX_CENTI - TARGET_MIN_CENTI + 1);
        char sep = (rand() % 2) ? '.' : ',';
        snprintf(text, sizeof text, "%d%c%02d", (int)(v / 100), sep, (int)(v % 100));

        int32_t got = 0;
        TEST_ASSERT_EQUAL_INT_MESSAGE(PARSE_OK, parse_target_centi(text, &got), text);
        TEST_ASSERT_EQUAL_INT32_MESSAGE(v, got, text);
    }
}

/* Satunnaista roskaa: ei saa kaatua, ja jos se hyväksytään, arvon pitää olla alueella. */
static void test_random_garbage(void)
{
    srand(777);
    static const char alphabet[] = "0123456789.,-+ ae";
    char text[12];
    for (int n = 0; n < 100000; n++) {
        size_t len = (size_t)(rand() % 11);
        for (size_t k = 0; k < len; k++) {
            text[k] = alphabet[rand() % (int)(sizeof alphabet - 1)];
        }
        text[len] = '\0';

        int32_t got = -12345;
        enum parse_result r = parse_target_centi(text, &got);
        if (r == PARSE_OK) {
            TEST_ASSERT_TRUE_MESSAGE(got >= TARGET_MIN_CENTI && got <= TARGET_MAX_CENTI, text);
        } else {
            TEST_ASSERT_EQUAL_INT32_MESSAGE(-12345, got, text);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_table);
    RUN_TEST(test_random_roundtrip);
    RUN_TEST(test_random_garbage);
    return UNITY_END();
}
