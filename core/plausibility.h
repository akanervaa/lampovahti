/* plausibility.h: onko anturin lukema fyysisesti mahdollinen (F8). Ydinmoduuli.
 *
 * Anturi voi vastata mutta väärin (bittivirhe, jumiutunut arvo). F7 huomaa vain
 * vastaamattoman anturin; tämä huomaa väärin vastaavan.
 *
 * Lukema on uskottava, kun MOLEMMAT pätevät:
 *   1. Alue:  PLAUS_MIN_CENTI <= lukema <= PLAUS_MAX_CENTI  (rajat itse ovat sallittuja)
 *   2. Hyppy: |lukema - edellinen| <= PLAUS_MAX_JUMP_CENTI  (vain jos edellinen on olemassa)
 *
 * "Edellinen" on edellinen LUKEMA, myös hylätty, eikä viimeisin hyväksytty. Muuten
 * oikea, äkillinen muutos (anturi siirretään lämpimään) hylättäisiin ikuisesti, koska
 * jokaista uutta lukemaa verrattaisiin vanhaan arvoon. Hinta: yksi bittivirhe hylkää
 * kaksi lukemaa (virheellisen ja paluun takaisin).
 *
 * Ensimmäisellä lukemalla edellistä ei ole, joten vain alue tarkistetaan.
 */
#ifndef PLAUSIBILITY_H
#define PLAUSIBILITY_H

#include <stdbool.h>
#include <stdint.h>

#define PLAUS_MIN_CENTI       (-4000)   /* -40.00 °C (F8) */
#define PLAUS_MAX_CENTI         8500    /* +85.00 °C (F8) */
#define PLAUS_MAX_JUMP_CENTI     200    /* 2.00 °C lukemien välillä (vaihe 5) */

bool plaus_ok(int32_t centi, bool have_prev, int32_t prev_centi);

#endif /* PLAUSIBILITY_H */
