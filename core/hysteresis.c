/* hysteresis.c: lämmittimen hystereesi. Sääntö ja perustelut: hysteresis.h.
 *
 * TDD-vaihe VIHREÄ: täytä aukot ____ niin, että kaikki 10 testiriviä menevät läpi.
 */
#include "hysteresis.h"

bool hysteresis_heater(bool heater_on, int32_t temp_centi,
                       int32_t target_centi, int32_t band_centi)
{
    int32_t low  = target_centi - band_centi / 2;
    int32_t high = target_centi + band_centi / 2;

    if (temp_centi < low) {
        return true;            /* kylmä: päälle */
    }
    if (temp_centi > high) {  /* vertailu: mieti rajatapausta 2285 */
        return false;            /* kuuma: pois */
    }
    return heater_on;                /* kaistalla (myös tasan rajalla): ? */
}
