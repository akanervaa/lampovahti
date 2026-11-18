/* mys1.c: ks. mys1.h. Rekisterikartta: MYS-1:n datalehti (lesson 63), kohta 3.1. */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#include "mys1.h"
#include "mys1_conv.h"

#define REG_WHO_AM_I    0x0F
#define REG_CTRL        0x20
#define REG_STATUS      0x27
#define REG_TEMP_L      0x28    /* TEMP_L ja TEMP_H luetaan yhdellä kertaa (0x28, 0x29) */

#define WHO_AM_I_VALUE  0xA7
#define CTRL_MEAS_EN    0x01    /* CTRL bitti 0: mittaus päälle */
#define STATUS_DRDY     0x01    /* STATUS bitti 0: uusi näyte valmis */

#define DRDY_POLL_MS    10      /* datalehti: "poll STATUS every few milliseconds" */

/* Anturi devicetreestä labelilla (boards/rpi_pico.overlay). */
static const struct i2c_dt_spec mys1 = I2C_DT_SPEC_GET(DT_NODELABEL(mys1));

int mys1_init(void)
{
    if (!i2c_is_ready_dt(&mys1)) {
        return -ENODEV;
    }

    uint8_t id;
    if (i2c_reg_read_byte_dt(&mys1, REG_WHO_AM_I, &id) != 0) {
        return -EIO;
    }
    if (id != WHO_AM_I_VALUE) {
        return -ENODEV;                     /* fail fast: väärä piiri */
    }

    return i2c_reg_write_byte_dt(&mys1, REG_CTRL, CTRL_MEAS_EN);
}

int mys1_read_centi(int32_t *centi)
{
    uint32_t start = k_uptime_get_32();

    /* Odota DRDY:tä: tarkista, nuku hetki, tarkista uudelleen. Nukkuessa muut threadit ajavat. */
    while (1) {
        uint8_t status;
        if (i2c_reg_read_byte_dt(&mys1, REG_STATUS, &status) != 0) {
            return -EIO;
        }

        /* (2) Onko DRDY-bitti ylhäällä? Maskaa status-rekisteristä oikea bitti
         *     (sama tekniikka kuin watchdog-kokeilun ENABLE-bitissä). */
        if ((status & STATUS_DRDY) != 0) {
            break;
        }

        /* Wrap-turvallinen aikavertailu (lesson 91): etumerkitön vähennys toimii, vaikka
         * millisekuntilaskuri pyörähtäisi ympäri. */
        if ((uint32_t)(k_uptime_get_32() - start) > CONFIG_LAMPO_DRDY_TIMEOUT_MS) {
            return -ETIMEDOUT;              /* anturi ei tuottanut näytettä: anturivika (F7) */
        }

        k_msleep(DRDY_POLL_MS);
    }

    /* Lue TEMP_L ja TEMP_H yhdellä lukukerralla. TEMP_L lukitsee näytteen, joten tavut ovat
     * samasta näytteestä; TEMP_H:n luku laskee DRDY:n. */
    uint8_t raw[2];
    if (i2c_burst_read_dt(&mys1, REG_TEMP_L, raw, sizeof raw) != 0) {
        return -EIO;
    }

    /* Kaksi tavua yhdeksi etumerkilliseksi 16-bittiseksi luvuksi: high-tavu ylös, low alas. */
    int16_t counts = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);

    /* (3) Muunna asteiksi YTIMEN funktiolla: se, jonka kirjoitit TDD:llä ja testasit kaikilla
     *     65 536 syötteellä. Tämä rivi on ytimen ja kuoren saumakohta. */
    *centi = mys1_raw_to_centi(counts);
    return 0;
}
