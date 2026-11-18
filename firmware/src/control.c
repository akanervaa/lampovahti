/* control.c: control-thread (DESIGN.md 4.1–4.3). Kuori ytimen decide()-funktion ympärillä.
 *
 * Kuori TEKEE: odottaa postia, kysyy ajan, sytyttää LEDin, lokittaa.
 * Ydin PÄÄTTÄÄ: decide() (core/controller.c), testattu PC:llä.
 *
 * Täytä aukot ____ ja vastaa kysymyksiin K1–K4 kommentteihin. API-nimet on annettu
 * kysymysten yhteydessä; mieti, mikä niistä sopii ja miksi.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "control.h"
#include "controller.h"

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

#define CONTROL_STACK_SIZE  1024    /* tavuina, mitataan myöhemmin (lesson 91) */
#define INBOX_DEPTH         8       /* tapahtumaa */
#define TARGET_CENTI        2270    /* 22.70 °C: Wokwin MYS-1 vaihtelee 22.6–22.9, joten LED räpsyy */

/* Postilaatikko: varataan käännösaikana (ei mallocia). Viestin koko on struct eventin koko,
 * ja viimeinen luku on tasaus tavuina. */
K_MSGQ_DEFINE(inbox, sizeof(struct event), INBOX_DEPTH, 4);

/* Lämmitin-LED devicetreestä (boards/rpi_pico.overlay, label heater_led). */
static const struct gpio_dt_spec heater_led = GPIO_DT_SPEC_GET(DT_NODELABEL(heater_led), gpios);

int control_post(struct event ev)
{
    /* K1: Aikakatkaisu: K_NO_WAIT (ei odota) vai K_FOREVER (odottaa, kunnes laatikossa on tilaa)?
     *     Mieti: mitä sensor-threadille tapahtuu, jos control on jumissa ja laatikko täynnä?
     *     Ja vaiheessa 6 tätä kutsuu napin KESKEYTYS. (DESIGN.md 4.2, "Täysi postilaatikko")
     *     Vastaus:
     */
    return k_msgq_put(&inbox, &ev, K_NO_WAIT);
}

static void control_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (!gpio_is_ready_dt(&heater_led)) {
        LOG_ERR("lammitin-LED ei valmis");
        return;
    }
    /* Lähtötila: GPIO_OUTPUT_ACTIVE (pinni heti päälle) vai GPIO_OUTPUT_INACTIVE (heti pois)?
     * Mieti: mitä lämmitin tekee sinä aikana, kun ensimmäistä lukemaa vielä odotetaan? (F7) */
    int rc = gpio_pin_configure_dt(&heater_led, GPIO_OUTPUT_INACTIVE);
    if (rc != 0) {
        LOG_ERR("lammitin-LEDin asetus epaonnistui (%d)", rc);
        return;
    }

    /* K2: Tila on control_threadin PAIKALLINEN muuttuja eikä globaali. Mitä etua siitä on?
     *     (DESIGN.md 4.2: "yksi kirjoittaja")
     *     Vastaus: No just se, että sillä on vaan yksi kirjoittaja ja se on paikallinen, niin ei tarvita lukituksia tai muuta monimutkaista synkronointia. */
    struct state st = { .target_centi = TARGET_CENTI };
    bool heater_was_on = false;

    LOG_INF("control kaynnissa, tavoite %d.%02d C", TARGET_CENTI / 100, TARGET_CENTI % 100);

    while (1) {
        struct event ev;
        /* Vaihe 4: odotetaan ikuisesti. Vaihe 5/8: K_MSEC(2500), jotta hiljaisuus huomataan. */
        k_msgq_get(&inbox, &ev, K_FOREVER);

        /* K3: Miksi tila annetaan osoitteena (&st) mutta tapahtuma arvona (ev)?
         *     Mieti: kumpaa decide() muuttaa, ja minkä muutosten pitää säilyä seuraavaan kertaan?
         *     Vastaus: tila annetaan osoitteena, koska decide voi muuttaa sen tilaa, mutta tapahtumaa ei anneta osoitteena, koska decide ei muuta sitä. Tapahtuma on vain syöte, eikä sen muutosten tarvitse säilyä seuraavaan kertaan. */
        struct actions act = decide(&st, ev, k_uptime_get_32());

        gpio_pin_set_dt(&heater_led, act.heater_on);
        if (act.heater_on != heater_was_on) {
            LOG_INF("lammitin %s", act.heater_on ? "PAALLE" : "POIS");
            heater_was_on = act.heater_on;
        }
    }
}

/* K4: Prioriteetti DESIGN.md 4.4:stä. Pienempi numero = kiireellisempi.
 *     Miksi controlin pitää olla kiireellisempi kuin sensorin (4)?
 *     Vastaus:
 *     Vastaus: Koska control-thread on vastuussa lämmitin-LEDin ohjauksesta, se pitää olla kiireellisempi kuin sensor-thread, jotta lämmitin voidaan ohjata nopeasti.
 */
K_THREAD_DEFINE(control_tid, CONTROL_STACK_SIZE, control_thread, NULL, NULL, NULL,
                K_PRIO_PREEMPT(2), 0, 0);
