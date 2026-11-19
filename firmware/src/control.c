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
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#include "control.h"
#include "controller.h"

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

#define CONTROL_STACK_SIZE  1024    /* tavuina, mitataan myöhemmin (lesson 91) */
#define INBOX_DEPTH         8       /* tapahtumaa */
#define WAKE_MS             1000    /* control herää vähintään näin usein, vaikka postia ei tule */
#define TARGET_CENTI        2270    /* 22.70 °C: Wokwin MYS-1 kiertää 22.50–22.93, joten LED vaihtuu */

/* Postilaatikko: varataan käännösaikana (ei mallocia). Viestin koko on struct eventin koko,
 * ja viimeinen luku on tasaus tavuina. */
K_MSGQ_DEFINE(inbox, sizeof(struct event), INBOX_DEPTH, 4);

/* LEDit devicetreestä (boards/rpi_pico.overlay). */
static const struct gpio_dt_spec heater_led = GPIO_DT_SPEC_GET(DT_NODELABEL(heater_led), gpios);
static const struct gpio_dt_spec alarm_led  = GPIO_DT_SPEC_GET(DT_NODELABEL(alarm_led), gpios);

/* Hälytys-LED vilkkuu 2 Hz k_timerilla (DESIGN.md 4.4): ei omaa threadia, koska vilkutus
 * on vain pinnin kääntö. Ajastimen funktio ajetaan keskeytyksessä, joten se ei saa odottaa;
 * gpio_pin_toggle_dt ei odota. */
#define ALARM_BLINK_MS  250     /* 250 ms päällä + 250 ms pois = 2 Hz */

static void alarm_blink(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    gpio_pin_toggle_dt(&alarm_led);
}

K_TIMER_DEFINE(alarm_timer, alarm_blink, NULL);

/* Vahtikoira (F9, DESIGN.md 4.5). Laudan devicetree antaa sille aliaksen watchdog0. */
static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

/* Käynnistää vahtikoiran. Palauttaa kanavan numeron (>= 0) tai virheen (< 0). */
static int watchdog_start(void)
{
    if (!device_is_ready(wdt)) {
        return -ENODEV;
    }
    struct wdt_timeout_cfg cfg = {
        .window.min = 0,                        /* ruokkia saa milloin tahansa */
        .window.max = CONFIG_LAMPO_WDT_TIMEOUT_MS,
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC,            /* koko piiri: myös GPIO:t nollautuvat → lämmitin pois */
    };
    int ch = wdt_install_timeout(wdt, &cfg);
    if (ch < 0) {
        return ch;
    }
    /* Debuggerin pysäyttäessä vahtikoira pysähtyy, muuten jokainen breakpoint olisi reset. */
    int rc = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    return (rc < 0) ? rc : ch;
}

/* Valokuva shellille. Koko järjestelmän AINOA mutex: se suojaa vain tätä kopiota, ja kumpikin
 * pitää sitä vain kopioinnin ajan (noin 0.5 kt memcpy, mikrosekunteja). */
static struct state snap;
K_MUTEX_DEFINE(snap_lock);

void control_snapshot(struct state *out)
{
    k_mutex_lock(&snap_lock, K_FOREVER);
    *out = snap;
    k_mutex_unlock(&snap_lock);
}

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

    if (!gpio_is_ready_dt(&heater_led) || !gpio_is_ready_dt(&alarm_led)) {
        LOG_ERR("LEDit eivat valmiita");
        return;
    }
    /* Lähtötila: GPIO_OUTPUT_ACTIVE (pinni heti päälle) vai GPIO_OUTPUT_INACTIVE (heti pois)?
     * Mieti: mitä lämmitin tekee sinä aikana, kun ensimmäistä lukemaa vielä odotetaan? (F7) */
    int rc = gpio_pin_configure_dt(&heater_led, GPIO_OUTPUT_INACTIVE);
    if (rc == 0) {
        rc = gpio_pin_configure_dt(&alarm_led, GPIO_OUTPUT_INACTIVE);
    }
    if (rc != 0) {
        LOG_ERR("LEDien asetus epaonnistui (%d)", rc);
        return;
    }

    /* Tila on näkyvissä vain tässä funktiossa: yksi kirjoittaja, joten lukkoja ei tarvita
     * (DESIGN.md 4.2). static: tila on noin 0.5 kt (historia 32 riviä), joten se ei mahdu
     * threadin 1024 tavun pinoon. static sijoittaa sen RAMiin käännösaikana; näkyvyys
     * pysyy silti vain tässä funktiossa. Alustus tehdään kerran, ei jokaisella kutsulla. */
    static struct state st = {
        .target_centi = TARGET_CENTI,
        .band_centi   = CONFIG_LAMPO_HYST_BAND_CENTI,
        .alarm_range_centi = CONFIG_LAMPO_ALARM_RANGE_CENTI,
        .alarm_delay_ms    = CONFIG_LAMPO_ALARM_DELAY_MS,
        .silence_ms        = CONFIG_LAMPO_SILENCE_MS,
        .heater_on    = false,          /* sama kuin LEDin lähtötila (INACTIVE) */
    };
    bool heater_was_on = false;
    bool alarm_was_on  = false;

    k_mutex_lock(&snap_lock, K_FOREVER);
    snap = st;                  /* shell näkee asetukset jo ennen ensimmäistä lukemaa */
    k_mutex_unlock(&snap_lock);

    /* Vahtikoira käynnistetään vasta, kun LEDit ovat turvallisessa tilassa (pois). Jos se ei
     * käynnisty, jatketaan ilman: laite toimii, mutta toinen turvakerros puuttuu, ja se näkyy
     * lokissa virheenä. */
    int wdt_ch = watchdog_start();
    if (wdt_ch < 0) {
        LOG_ERR("vahtikoira ei kaynnistynyt (%d)", wdt_ch);
    } else {
        LOG_INF("vahtikoira paalla, aikaraja %d ms", CONFIG_LAMPO_WDT_TIMEOUT_MS);
    }

    LOG_INF("control kaynnissa, tavoite %d.%02d C", TARGET_CENTI / 100, TARGET_CENTI % 100);

    while (1) {
        struct event ev;
        /* Odotetaan postia enintään WAKE_MS. Jos postia ei tule, decide() saa EV_TICKin: se
         * tarkistaa, onko sensor hiljaa, ja silmukka ehtii ruokkia vahtikoiran. K_FOREVER
         * jumittaisi tähän, jos sensor kuolee, ja vahtikoira käynnistäisi laitteen uudelleen
         * yhä uudestaan ilman hälytystä tai lokia (DESIGN.md 4.5, hylätyt). */
        if (k_msgq_get(&inbox, &ev, K_MSEC(WAKE_MS)) != 0) {
            ev = (struct event){ .type = EV_TICK, .centi = 0 };
        }

        /* K3: Miksi tila annetaan osoitteena (&st) mutta tapahtuma arvona (ev)?
         *     Mieti: kumpaa decide() muuttaa, ja minkä muutosten pitää säilyä seuraavaan kertaan?
         *     Vastaus: tila annetaan osoitteena, koska decide voi muuttaa sen tilaa, mutta tapahtumaa ei anneta osoitteena, koska decide ei muuta sitä. Tapahtuma on vain syöte, eikä sen muutosten tarvitse säilyä seuraavaan kertaan. */
        struct actions act = decide(&st, ev, k_uptime_get_32());

        k_mutex_lock(&snap_lock, K_FOREVER);
        snap = st;
        k_mutex_unlock(&snap_lock);

        gpio_pin_set_dt(&heater_led, act.heater_on);
        if (act.heater_on != heater_was_on) {
            LOG_INF("lammitin %s", act.heater_on ? "PAALLE" : "POIS");
            heater_was_on = act.heater_on;
        }

        /* Hälytys: kuori toteuttaa vilkutuksen ja kirjaa muutoksen lokiin (F6). */
        if (act.alarm_on != alarm_was_on) {
            if (act.alarm_on) {
                LOG_WRN("HALYTYS");
                k_timer_start(&alarm_timer, K_MSEC(ALARM_BLINK_MS), K_MSEC(ALARM_BLINK_MS));
            } else {
                LOG_INF("halytys kuitattu");
                k_timer_stop(&alarm_timer);
                gpio_pin_set_dt(&alarm_led, 0);
            }
            alarm_was_on = act.alarm_on;
        }

        /* Ruokinta VAIN täällä, kierroksen lopussa: ruokinta todistaa, että koko kierros
         * (odotus, päätös, LEDit) meni läpi. Erillinen ruokintathread ruokkisi, vaikka tämä
         * silmukka olisi jumissa (DESIGN.md 4.5, "ruokkijan pitää olla se, jonka toiminta
         * halutaan todistaa"). */
        if (wdt_ch >= 0) {
            wdt_feed(wdt, wdt_ch);
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
