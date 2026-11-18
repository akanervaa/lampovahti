/* hysteresis.h: lämmittimen hystereesi (F2). Ydinmoduuli: puhdasta C:tä, ei Zephyriä.
 *
 * Kuollut kaista tavoitteen ympärillä: lämmitin syttyy alarajan alapuolella ja sammuu
 * ylärajan yläpuolella. Kaistan sisällä jatketaan entisellä tavalla, joten pieni heilunta
 * tavoitteen ympärillä ei räpsytä relettä.
 *
 *   alaraja = tavoite - kaista/2      ylaraja = tavoite + kaista/2
 *
 * Vertailut ovat aitoja (< ja >): tasan rajalla ollaan vielä kaistan sisällä.
 *
 * Funktio ei muista mitään itse: edellinen tila annetaan parametrina (heater_on), ja
 * decide() pitää sen muistissa struct statessa. Sama syöte antaa aina saman tuloksen.
 */
#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include <stdbool.h>
#include <stdint.h>

bool hysteresis_heater(bool heater_on, int32_t temp_centi,
                       int32_t target_centi, int32_t band_centi);

#endif /* HYSTERESIS_H */
