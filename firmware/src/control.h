/* control.h: control-threadin postilaatikko (DESIGN.md 4.2).
 *
 * Muut threadit (ja myöhemmin napin keskeytys) eivät näe postilaatikkoa suoraan: ne kutsuvat
 * control_post(). Näin laatikon koko, odotusaika ja pudotusten käsittely ovat yhdessä paikassa.
 */
#ifndef CONTROL_H
#define CONTROL_H

#include "event.h"

/* Lähettää tapahtuman control-threadille. Ei odota koskaan.
 * Palauttaa 0, tai negatiivisen virhekoodin, jos laatikko oli täynnä (tapahtuma pudotettiin). */
int control_post(struct event ev);

#endif /* CONTROL_H */
