/* plausibility.c: anturin lukeman uskottavuus. Säännöt ja perustelut: plausibility.h.
 *
 * TDD-vaihe VIHREÄ: täytä aukot ____ niin, että kaikki 11 testiriviä menevät läpi.
 */
#include "plausibility.h"

bool plaus_ok(int32_t centi, bool have_prev, int32_t prev_centi)
{
    /* 1. Alue. Rajat itse ovat sallittuja: mitkä vertailuoperaattorit? */
    if (centi < PLAUS_MIN_CENTI || centi > PLAUS_MAX_CENTI) {
        return false;
    }

    /* 2. Hyppy, vain jos edellinen lukema on olemassa. */
    if (have_prev) {
        int32_t jump = centi - prev_centi;
        if (jump < 0) {
            jump = -jump;        /* itseisarvo: alas 201 on yhtä mahdoton kuin ylös 201 */
        }
        if (jump > PLAUS_MAX_JUMP_CENTI) {
            return false;
        }
    }

    return true;
}
