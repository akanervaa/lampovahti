/* mys1.h: MYS-1-anturin ajuri (kuori: I2C, DRDY-odotus, aikakatkaisu).
 *
 * Muunnos raakaluvusta asteiksi on ytimessä (core/mys1_conv.h), koska se ei tarvitse Zephyriä
 * ja on testattu PC:llä. Tämä tiedosto hoitaa vain sen, mikä vaatii rautaa.
 */
#ifndef MYS1_H
#define MYS1_H

#include <stdint.h>

/* Tarkistaa WHO_AM_I:n (fail fast) ja käynnistää mittauksen.
 * Palauttaa 0, tai negatiivisen virhekoodin: -ENODEV (väylä ei valmis tai väärä piiri),
 * -EIO (väylävirhe). */
int mys1_init(void);

/* Odottaa uutta näytettä (DRDY) enintään CONFIG_LAMPO_DRDY_TIMEOUT_MS ja lukee sen.
 * Odotus nukkuu tarkistusten välissä, joten muut threadit saavat CPU:n.
 * Palauttaa 0 ja kirjoittaa lämpötilan sadasosa-asteina *centi:iin, tai negatiivisen
 * virhekoodin: -ETIMEDOUT (DRDY ei noussut = anturivika, F7), -EIO (väylävirhe). */
int mys1_read_centi(int32_t *centi);

#endif /* MYS1_H */
