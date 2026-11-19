/* cmd_parse.c: tavoitelämpötila tekstistä sadasosiksi. Säännöt: cmd_parse.h.
 *
 * TDD-vaihe VIHREÄ: täytä aukot ____ niin, että test_cmd_parse menee läpi.
 *
 * Teksti käydään läpi merkki kerrallaan osoittimella p, vasemmalta oikealle:
 *     "-23.05"
 *      ^         etumerkki (valinnainen)
 *       ^^       kokonaisosa (vähintään yksi numero)
 *         ^      piste tai pilkku (valinnainen)
 *          ^^    1–2 desimaalia
 *            ^   loppumerkki '\0': muuten syntaksivirhe
 */
#include <stdbool.h>

#include "cmd_parse.h"

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

enum parse_result parse_target_centi(const char *text, int32_t *out_centi)
{
    const char *p = text;

    if (*p == '\0') {
        return PARSE_EMPTY;
    }

    bool negative = false;
    if (*p == '-') {
        negative = true;
        p++;
    }

    /* Kokonaisosa. */
    if (!is_digit(*p)) {
        return PARSE_SYNTAX;                /* "abc", ".5", "+23", "--5" */
    }
    int32_t whole = 0;
    bool too_big = false;
    while (is_digit(*p)) {
        if (!too_big) {
            /* (1) Merkki '7' ei ole luku 7: se on ASCII-koodi 55. Merkit '0'...'9' ovat
             *     peräkkäin (48...57). Mitä merkistä pitää vähentää, jotta saadaan numeron arvo? */
            whole = whole * 10 + (*p - '0');

            /* (2) Ylivuotosuoja. Ilman tätä "99999999999" pyörähtäisi int32_t:ssä ympäri ja
             *     voisi osua sallitulle alueelle. Valitse raja, joka on varmasti yli 35 mutta
             *     niin pieni, ettei whole * 10 + 9 vuoda yli seuraavalla kierroksella. */
            if (whole > 50000) {
                too_big = true;             /* luetaan silti loppuun: "999999abc" on syntaksivirhe */
            }
        }
        p++;
    }

    /* Desimaaliosa. */
    int32_t frac = 0;
    if (*p == '.' || *p == ',') {
        p++;
        if (!is_digit(*p)) {
            return PARSE_SYNTAX;            /* "23." */
        }
        /* (3) Ensimmäinen desimaali on kymmenyksiä: "23.5" on 23 ja 50 sadasosaa, ei 5.
         *     Millä kerrotaan? */
        frac = (*p - '0') * 10;
        p++;
        if (is_digit(*p)) {
            frac += *p - '0';               /* toinen desimaali: sadasosat sellaisenaan */
            p++;
        }
    }

    if (*p != '\0') {
        return PARSE_SYNTAX;                /* "23.555", "23.5abc", "2 3" */
    }
    if (too_big) {
        return PARSE_RANGE;
    }

    int32_t centi = whole * 100 + frac;
    if (negative) {
        centi = -centi;
    }

    /* (4) Alue: rajat itse ovat sallittuja (testi "35" → PARSE_OK). Valitse vertailut: < <= > >= */
    if (centi < TARGET_MIN_CENTI || centi > TARGET_MAX_CENTI) {
        return PARSE_RANGE;
    }

    *out_centi = centi;                     /* kirjoitetaan vain onnistuessa */
    return PARSE_OK;
}
