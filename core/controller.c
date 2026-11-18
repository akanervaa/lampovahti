/* controller.c: VAIHE 4 -tynkä decide()-funktiosta.
 *
 * Täytä aukot ____ ja vastaa kysymyksiin K1–K3 kommentteihin.
 */
#include "controller.h"

struct actions decide(struct state *s, struct event ev, uint32_t now_ms)
{
    /* K1: Miksi oletusvastaus on "lämmitin pois" eikä "päällä"? (DESIGN.md F7)
     *     Vastaus: Oletusvastaus on "lämmitin pois", koska on turvallisempaa jättää lämmitin pois päältä, jos anturi ei toimi oikein. Tämä vähentää riskiä ylikuumentumisesta tai muusta haitallisesta toiminnasta.
     */
    struct actions act = { .heater_on = false };

    /* K2: Tynkä ei käytä aikaa. Ilman tätä riviä PC-testien käännös kaatuu.
     *     Miksi? (Vinkki: katso tests/CMakeLists.txt:n kääntäjäliput.)
     *     Vastaus: aika on määritelty muuttujaksi, mutta sitä ei käytetä. Kääntäjä varoittaa käyttämättömästä muuttujasta, ja koska käännös on asetettu varoituksista virheiksi (esim. -Werror), käännös kaatuu. Tämä rivi estää varoituksen käyttämättömästä muuttujasta.
     */
    (void)now_ms;

    switch (ev.type) {
    case EV_SAMPLE:
        act.heater_on = ev.centi < s->target_centi;   /* tynkäsääntö: lämpötila < tavoite */
        break;

    case EV_SENSOR_FAULT:
        act.heater_on = false;
        break;

    /* K3: Tässä switchissä ei ole default-haaraa. Miksi se on tarkoituksellista?
     *     (Muista event.h:n kommentti enumista.)
     *     Vastaus: Koska käytetään enum, kääntäjä voi varoittaa, jos jotain enum-arvoa ei käsitellä switchissä.
     */
    }

    return act;
}
