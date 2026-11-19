/* event.h: tapahtumat, jotka control-thread saa postilaatikosta ja antaa decide()-funktiolle.
 *
 * Ydinmoduuli: puhdasta C:tä, ei Zephyriä (DESIGN.md 4.3). Event on ytimessä, koska decide()
 * on ytimessä ja ottaa sen parametrina. Riippuvuus kulkee vain kuoresta ytimeen.
 *
 * Tyypit ovat enum eivätkä #define-vakioita: kun switch(ev.type) unohtaa jonkin tyypin,
 * kääntäjä varoittaa (-Wswitch, osa -Wall:ia). Uutta tyyppiä lisättäessä kääntäjä siis
 * näyttää jokaisen kohdan, joka pitää päivittää.
 */
#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

enum event_type {
    EV_SAMPLE,        /* uusi lämpötilanäyte, arvo kentässä centi */
    EV_SENSOR_FAULT,  /* anturi ei vastannut tai DRDY ei noussut ajoissa (F7).
                       * centi ei merkitse mitään: decide() ei lue sitä, lähettäjä asettaa 0. */
    EV_BUTTON,        /* kuittausnappia painettiin (F4), värähtely jo suodatettu kuoressa
                       * (firmware/src/button.c). centi ei merkitse mitään, lähettäjä asettaa 0. */
    EV_SET_TARGET,    /* uusi tavoite shellistä (F5), arvo kentässä centi. Lähettäjä on jo
                       * tarkistanut arvon (core/cmd_parse.c). */
};

struct event {
    enum event_type type;
    /* Lämpötila sadasosa-asteina (2250 = 22.50 °C), sama muoto kuin mys1_raw_to_centi palauttaa.
     * Tyyppi on sama kuin mys1_raw_to_centi:n paluuarvo, koska tulos (enintään ±51200)
     * ei mahdu int16_t:hen: 51198 katkeaisi hiljaa luvuksi -14338. */
    int32_t centi;
};

#endif /* EVENT_H */
