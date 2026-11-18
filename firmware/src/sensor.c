/* sensor.c: sensor-thread (DESIGN.md 4.1). Lukee lämpötilan jaksollisesti.
 *
 * Tällä hetkellä lämpötila kirjataan lokiin. Myöhemmin (vaihe 4) lukema lähetetään
 * control-threadin postilaatikkoon (EV_SAMPLE) ja vika viestinä EV_SENSOR_FAULT.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "mys1.h"

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_INF);

#define SENSOR_STACK_SIZE  1024     /* tavuina (Zephyr), mitataan myöhemmin (lesson 91) */

static void sensor_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int rc = mys1_init();
    if (rc != 0) {
        LOG_ERR("MYS-1 alustus epaonnistui (%d)", rc);
        return;
    }
    LOG_INF("MYS-1 ok, mittaus kaynnissa");

    /* Jakso absoluuttisella ajalla ("sleep until", kuten FreeRTOSin vTaskDelayUntil):
     * seuraava herätys lasketaan edellisestä TAVOITTEESTA, ei siitä milloin herättiin,
     * joten luvun kesto ja Wokwin unen venyminen eivät kerry. */
    int64_t next = k_uptime_get();

    while (1) {
        int32_t centi;
        rc = mys1_read_centi(&centi);
        if (rc == 0) {
            /* Etumerkki erikseen: -0.50 °C = -50 sadasosaa, ja -50 / 100 = 0 hävittäisi miinuksen. */
            int32_t mag = (centi < 0) ? -centi : centi;
            LOG_INF("T = %s%d.%02d C", (centi < 0) ? "-" : "", (int)(mag / 100), (int)(mag % 100));
        } else {
            LOG_ERR("anturivika (%d)", rc);
        }

        /* (4) Seuraava herätys: yksi jakso edellisestä tavoitteesta. Mikä Kconfig-symboli? */
        next += CONFIG_LAMPO_SENSOR_PERIOD_MS;
        k_sleep(K_TIMEOUT_ABS_MS(next));
    }
}

/* (5) Prioriteetti: katso DESIGN.md 4.4. Muista: Zephyrissä pienempi numero = kiireellisempi. */
K_THREAD_DEFINE(sensor_tid, SENSOR_STACK_SIZE, sensor_thread, NULL, NULL, NULL,
                K_PRIO_PREEMPT(4), 0, 0);
