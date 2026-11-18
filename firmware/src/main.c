/* main.c: Lämpövahdin käynnistys.
 *
 * Vaihe 3, osa 2a: todistetaan, että MYS-1-anturi vastaa Wokwissa (WHO_AM_I).
 * Lämpötilan lukeminen ja sensor-thread tulevat seuraavassa osassa.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* MYS-1:n datalehti (lesson 63), kohta 3.1. Kirjoita arvot datalehden mukaan. */
#define MYS1_REG_WHO_AM_I    0x0F    /* (4) WHO_AM_I-rekisterin osoite */
#define MYS1_WHO_AM_I_VALUE  0xA7    /* (5) arvo, joka sieltä pitää löytyä */

/* LÖYDÄ: anturi devicetreestä labelilla. Labelin pitää olla sama kuin overlayssa. */
static const struct i2c_dt_spec mys1 = I2C_DT_SPEC_GET(DT_NODELABEL(mys1));   /* (6) */

int main(void)
{
    LOG_INF("Lampovahti kaynnistyy");

    /* TARKISTA: käynnistyikö väylän ajuri? Jos ei, vika on Kconfigissa (aukko 3). */
    if (!i2c_is_ready_dt(&mys1)) {
        LOG_ERR("I2C-vayla ei valmis");
        return 0;
    }

    /* KÄYTÄ: lue WHO_AM_I. i2c_write_read_dt kirjoittaa rekisterin osoitteen ja lukee vastauksen
     * yhdessä transaktiossa (repeated start, lesson 96). */
    uint8_t reg = MYS1_REG_WHO_AM_I;
    uint8_t id = 0;
    int rc = i2c_write_read_dt(&mys1, &reg, 1, &id, 1);
    if (rc != 0) {
        LOG_ERR("MYS-1 ei vastaa (virhe %d)", rc);
        return 0;
    }

    /* (7) FAIL FAST: jos vastaaja ei ole MYS-1, älä jatka väärän piirin kanssa.
     *     Kirjoita ehto, joka on tosi, kun luettu id EI ole odotettu arvo. */
    if (id != MYS1_WHO_AM_I_VALUE) {
        LOG_ERR("vaara piiri: WHO_AM_I 0x%02x, odotettiin 0x%02x", id, MYS1_WHO_AM_I_VALUE);
        return 0;
    }

    LOG_INF("MYS-1 vastaa: WHO_AM_I 0x%02x", id);
    return 0;
}
