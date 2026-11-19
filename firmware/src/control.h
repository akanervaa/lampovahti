/* control.h: control-threadin postilaatikko (DESIGN.md 4.2).
 *
 * Muut threadit (ja myöhemmin napin keskeytys) eivät näe postilaatikkoa suoraan: ne kutsuvat
 * control_post(). Näin laatikon koko, odotusaika ja pudotusten käsittely ovat yhdessä paikassa.
 */
#ifndef CONTROL_H
#define CONTROL_H

#include "controller.h"

/* Lähettää tapahtuman control-threadille. Ei odota koskaan.
 * Palauttaa 0, tai negatiivisen virhekoodin, jos laatikko oli täynnä (tapahtuma pudotettiin). */
int control_post(struct event ev);

/* Valokuva: kopio control-threadin tilasta viimeisimmän tapahtuman jälkeen (DESIGN.md 4.2).
 * Shell lukee tätä, ei control-threadin omaa tilaa. Kopiointi tehdään mutexin takana, joten
 * kopio ei ole koskaan puoliksi päivitetty. Saa kutsua vain threadista (mutex voi odottaa). */
void control_snapshot(struct state *out);

#endif /* CONTROL_H */
