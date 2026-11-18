/* mys1_conv.h: MYS-1-anturin raakaluvun muunnos lämpötilaksi.
 *
 * Ydinmoduuli: puhdasta C:tä, ei Zephyriä (DESIGN.md 4.3). Testit: tests/test_mys1_conv.c.
 */
#ifndef MYS1_CONV_H
#define MYS1_CONV_H

#include <stdint.h>

/* Muuntaa MYS-1:n raakaluvun sadasosa-asteiksi (2250 = 22.50 °C).
 *
 * raw: anturin TEMP_H:TEMP_L, etumerkillinen 16-bittinen luku, 64 raakalukua = 1 °C
 *      (datalehti, kohta 4.1). Etumerkillinen tyyppi on pakollinen: muuten -64 (0xFFC0)
 *      tulkittaisiin luvuksi 65472 ja tulos olisi +1023 °C eikä -1.00 °C.
 *
 * Pyöristys: KATKAISU nollaa kohti (C:n kokonaislukujako). Esim. raw = 1 → 1.5625 → 1,
 * raw = -1 → -1.5625 → -1. Virhe on alle 0.01 °C, mikä on termostaatille merkityksetön,
 * ja katkaisu on yksinkertaisempi kuin pyöristys (päätös vaiheessa 3).
 *
 * Funktio muuntaa minkä tahansa raakaluvun. Se EI tarkista, onko lämpötila järkevä:
 * se on plausibility-moduulin tehtävä (F8).
 */
int32_t mys1_raw_to_centi(int16_t raw);

#endif /* MYS1_CONV_H */
