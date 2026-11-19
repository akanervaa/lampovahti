/* cmd_parse.h: shellin käyttäjän kirjoittama tavoitelämpötila tekstistä luvuksi (F5). Ydinmoduuli.
 *
 * Esimerkki: "23.5" → 2350 (sadasosa-asteina, sama muoto kuin muualla ytimessä).
 *
 * Säännöt (päätetty vaiheessa 7):
 *   - Kokonaisosa, ja halutessa piste TAI pilkku ja 1–2 desimaalia: "23", "23.5", "23,55".
 *     Pilkku hyväksytään, koska suomalainen käyttäjä kirjoittaa sen luonnostaan.
 *   - Kolme desimaalia ("23.555") hylätään eikä pyöristetä: laite ei saa hiljaa käyttää eri
 *     arvoa kuin käyttäjä kirjoitti.
 *   - Kaikki muu hylätään: kirjaimet, tyhjä, "23.", ".5", "+23", välilyönnit.
 *   - Sallittu alue TARGET_MIN..TARGET_MAX, rajat mukaan lukien. Huoneen termostaatti:
 *     alle 5 °C putket jäätyvät, yli 35 °C ei ole järkevä huonelämpötila. Alue on paljon
 *     kapeampi kuin anturin uskottava alue (-40...85 °C), jotta myös tavoite ± hälytysalue
 *     pysyy anturin alueella.
 *   - Valtavat luvut ("99999999999") eivät saa pyörähtää ympäri pieneksi luvuksi, vaan ne
 *     ovat alueen ulkopuolella.
 *
 * Ei käytä strtol/atof-funktioita: ne riippuvat localesta, ja ylivuodon käsittely on hankalaa.
 */
#ifndef CMD_PARSE_H
#define CMD_PARSE_H

#include <stdint.h>

#define TARGET_MIN_CENTI  500     /*  5.00 °C */
#define TARGET_MAX_CENTI 3500     /* 35.00 °C */

enum parse_result {
    PARSE_OK,       /* *out_centi asetettu */
    PARSE_EMPTY,    /* tyhjä merkkijono */
    PARSE_SYNTAX,   /* ei ole luku sääntöjen mukaan */
    PARSE_RANGE,    /* luku, mutta sallitun alueen ulkopuolella */
};

/* Palauttaa tuloksen. *out_centi kirjoitetaan VAIN, kun tulos on PARSE_OK. */
enum parse_result parse_target_centi(const char *text, int32_t *out_centi);

#endif /* CMD_PARSE_H */
