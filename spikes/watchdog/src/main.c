/* Watchdog-kokeilu (vaihe 0). Kertakäyttöinen.
 *
 * Kysymykset, joihin tämä vastaa:
 *   1. Käynnistyykö piiri Wokwissa uudelleen, kun watchdogia lakataan ruokkimasta?
 *   2. Onko Zephyrin ajoitus Wokwissa tarkka? (Nukutaan 1000 ms ja mitataan, paljonko kului.)
 *
 * Ohjelma ruokkii watchdogia neljä kertaa sekunnin välein ja lopettaa sitten ruokkimisen.
 * Uudelleenkäynnistyksen tunnistaa siitä, että "BOOT"-rivi tulostuu uudestaan.
 *
 * Täytä aukot (1)–(4). Aukko (1) on prj.conf-tiedostossa.
 */
#include <zephyr/sys/sys_io.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>

/* (2) Watchdogin aikaraja millisekunteina. Ruokimme sekunnin välein (PERIOD_MS).
 *     Mieti: pitääkö aikarajan olla pienempi vai suurempi kuin ruokintaväli, ja kuinka paljon?
 *     Mitä tapahtuisi, jos se olisi tasan 1000? */
#define WDT_TIMEOUT_MS 1500
#define FEEDS          4
#define PERIOD_MS      1000

/* LÖYDÄ: Picon laudan devicetree antaa watchdogille aliaksen "watchdog0". */
static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

int main(void)
{
    printk("BOOT at %lld ms\n", k_uptime_get());

    /* TARKISTA: käynnistyikö ajuri? Jos aukko (1) puuttuu, tämä epäonnistuu. */
    if (!device_is_ready(wdt)) {
        printk("watchdog not ready\n");
        return 0;
    }

    /* ASETA: watchdogin "kanava" eli yksi aikaraja.
     *   window.max = aikaraja: jos ruokintaa ei tule tämän ajan sisällä, watchdog laukeaa.
     *   window.min = 0: ruokkia saa milloin tahansa (jotkin piirit kieltävät liian aikaisen
     *                ruokinnan, mutta sitä emme tarvitse).
     *   flags = mitä tehdään, kun watchdog laukeaa. Vaihtoehdot:
     *     WDT_FLAG_RESET_NONE      ei mitään (vain callback, jos sellainen on)
     *     WDT_FLAG_RESET_CPU_CORE  käynnistä vain prosessoriydin uudelleen
     *     WDT_FLAG_RESET_SOC       käynnistä koko piiri uudelleen (SoC = system on chip)
     * (3) Kumpi näistä vastaa vaatimusta F9 ("piiri käynnistyy uudelleen, lämmitin pois")?
     *     Mieti: nollautuvatko GPIO-pinnit (lämmitin-LED), jos vain ydin käynnistyy uudelleen? */
    struct wdt_timeout_cfg cfg = {
        .window.min = 0,
        .window.max = WDT_TIMEOUT_MS,
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC,
    };
    int channel = wdt_install_timeout(wdt, &cfg);   /* palauttaa kanavan numeron tai virheen */
    if (channel < 0) {
        printk("install failed %d\n", channel);
        return 0;
    }

    /* Käynnistä watchdog. PAUSE_HALTED_BY_DBG: watchdog ei laukea, kun debuggeri on
     * pysäyttänyt ohjelman (muuten jokainen breakpoint käynnistäisi piirin uudelleen). */
    int rc = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (rc < 0) {
        printk("setup failed %d\n", rc);
        return 0;
    }

    /* KÄYTÄ: ruoki neljä kertaa ja mittaa samalla, paljonko 1000 ms:n uni oikeasti kestää. */
    int64_t prev = k_uptime_get();
    for (int i = 1; i <= FEEDS; i++) {
        int64_t now = k_uptime_get();
        k_msleep(PERIOD_MS);
        int64_t after = k_uptime_get();
        /* (4) Ruoki watchdogia. Funktio on wdt_feed, ja se tarvitsee kaksi tietoa:
         *     MINKÄ watchdog-laitteen ja MINKÄ kanavan. Molemmat ovat yllä muuttujina. */
        wdt_feed(wdt, channel);

        prev = now;
        printk("feed %d at %lld ms (slept %lld ms)\n", i, after, after- prev);
    }

    printk("stop feeding\n");
    while (1) {
        uint32_t ctrl = sys_read32(0x40058000);
        unsigned enable = (ctrl >> 30) & 1u; 
        unsigned time   = ctrl & 0xFFFFFFu;  
        k_msleep(PERIOD_MS);
        printk("still alive at %lld ms (enable: %u, time: %u)\n", k_uptime_get(), enable, time);
    }
    return 0;
}
