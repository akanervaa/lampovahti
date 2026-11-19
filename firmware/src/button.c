/* button.c: kuittausnappi (F4, DESIGN.md vaihe 6). Kuorta: keskeytys, ajastus ja pinnit.
 *
 * Kulku:
 *   painallus → keskeytys (button_isr) → odotetaan DEBOUNCE_MS → luetaan pinni uudelleen
 *            → jos yhä alhaalla: EV_BUTTON postilaatikkoon → control → decide() kuittaa.
 *
 * Värähtely suodatetaan täällä kuoressa, ei ISR:ssä eikä ytimessä:
 *   - ISR ei saa odottaa, joten se vain käynnistää ajastetun työn (k_work_delayable).
 *   - Ytimeen ei kannata viedä jokaista reunaa: yksi painallus antaa kymmeniä reunoja, ja ne
 *     täyttäisivät 8 paikan postilaatikon, jolloin sensorin lukemia putoaisi pois.
 * Työ ajetaan Zephyrin järjestelmän työjonossa (system workqueue), eli tavallisessa threadissa.
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "control.h"

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#define DEBOUNCE_MS 30      /* mekaanisen napin värähtely kestää tyypillisesti 5–20 ms */

/* Nappi devicetreestä (boards/rpi_pico.overlay): GP13, aktiivinen alhaalla, sisäinen pull-up.
 * GPIO_ACTIVE_LOW-lipun ansiosta gpio_pin_get_dt() palauttaa 1, kun nappi on POHJASSA,
 * vaikka pinni on silloin 0 V. Koodin ei tarvitse tietää kytkennän napaisuutta. */
static const struct gpio_dt_spec ack_button = GPIO_DT_SPEC_GET(DT_NODELABEL(ack_button), gpios);

static struct gpio_callback button_cb;

static void debounce_done(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(debounce_work, debounce_done);

/* Ajetaan työjonon threadissa DEBOUNCE_MS viimeisen reunan jälkeen. */
static void debounce_done(struct k_work *work)
{
    ARG_UNUSED(work);

    /* K2: Miksi pinni luetaan tässä vielä kerran? Keskeytyshän jo kertoi, että nappia painettiin.
     *     Mieti: mitä tapahtuu, kun nappi PÄÄSTETÄÄN irti? Sekin värähtelee.
     *     Vastaus: värähtelyn takia varmaa
     */
    if (gpio_pin_get_dt(&ack_button) != 1) {
        return;
    }

    struct event ev = { .type = EV_BUTTON, .centi = 0 };
    if (control_post(ev) != 0) {
        LOG_WRN("postilaatikko taynna, painallus hukattiin");
    }
}

/* Keskeytyskäsittelijä: lyhyt, ei odota mitään. */
static void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    /* K1: k_work_schedule ei tee mitään, jos työ on jo ajastettu: ensimmäinen määräaika jää
     *     voimaan. k_work_reschedule siirtää määräajan aina DEBOUNCE_MS eteenpäin tästä hetkestä.
     *     Kumpi sopii värähtelyyn, ja miksi? (Yksi painallus = noin 20 reunaa 10 ms aikana.)
     *     Vastaus: k_work_reschedule sopii, koska se varmistaa, että värähtely on päättynyt ennen kuin nappi käsitellään.
     */
    k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_MS));
}

/* Ajetaan kerran käynnistyksessä ennen main():ia (SYS_INIT alla). */
static int button_init(void)
{
    if (!gpio_is_ready_dt(&ack_button)) {
        LOG_ERR("nappi ei valmis");
        return -ENODEV;
    }

    int rc = gpio_pin_configure_dt(&ack_button, GPIO_INPUT);
    if (rc == 0) {
        /* Keskeytys reunasta "ei pohjassa → pohjassa". */
        rc = gpio_pin_interrupt_configure_dt(&ack_button, GPIO_INT_EDGE_TO_ACTIVE);
    }
    if (rc != 0) {
        LOG_ERR("napin asetus epaonnistui (%d)", rc);
        return rc;
    }

    gpio_init_callback(&button_cb, button_isr, BIT(ack_button.pin));
    gpio_add_callback(ack_button.port, &button_cb);
    return 0;
}

SYS_INIT(button_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
