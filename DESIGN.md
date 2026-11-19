# Lämpövahti: suunnitteludokumentti

Pieni lämpötilaa ohjaava laite (esim. terraario tai akvaario). Se mittaa lämpötilaa, ohjaa
lämmitintä ja hälyttää, jos jokin menee vikaan. Rakennettu tyhjästä Zephyrillä Raspberry Pi
Picolle, ajetaan Wokwi-simulaattorissa.

Tämä dokumentti kertoo **mitä** tehdään ja ennen kaikkea **miksi**. Jokaisessa päätöksessä
on kirjattu myös hylätyt vaihtoehdot ja hylkäämisen syy.

Tila: **vaihe 4, toteutus** (vaatimukset, arkkitehtuuri ja toteutusjärjestys valmiit 2026-11-18).

---

## 1. Laitteisto (Wokwi)

| Osa | Tehtävä | Huomio |
|---|---|---|
| Raspberry Pi Pico (RP2040) | Ohjain | Zephyr v4.5.0, lauta `rpi_pico` |
| MYS-1-anturi | Lämpötila | I2C, 7-bittinen osoite 0x48, WHO_AM_I = 0xA7, 64 raakalukua / °C |
| LED "lämmitin" | Esittää lämmittimen relettä | |
| LED "hälytys" | Vilkkuu hälytyksessä | |
| Painonappi | Kuittaa hälytyksen | Aktiivinen alhaalla, sisäinen pull-up |

Pinnit päätetään devicetreen overlayssa (`boards/rpi_pico.overlay`), ei C-koodissa (N2).

---

## 2. Vaatimukset

### Toiminnalliset

| # | Vaatimus | Miksi |
|---|---|---|
| F1 | Mittaa lämpötila kerran sekunnissa. | Lämpötila muuttuu hitaasti; 1 s riittää ja säästää väylää. |
| F2 | Ohjaa lämmitintä **hystereesillä**: päälle, kun T ≤ tavoite − 0.5 °C; pois, kun T ≥ tavoite + 0.5 °C; välissä pidä nykyinen tila. | Yksi raja kytkisi releen päälle ja pois jatkuvasti, kun lämpötila heiluu rajan kohdalla. Rele kuluisi ja aiheuttaisi häiriöitä. Ks. päätös P2. |
| F3 | Hälytä (hälytys-LED vilkkuu), jos lämpötila on liian kauan sallitun alueen ulkopuolella. | Lyhyt poikkeama (esim. kannen avaus) ei saa hälyttää turhaan. Arvot: ks. avoimet kysymykset. |
| F4 | Napin painallus kuittaa hälytyksen. | |
| F5 | Shell-komennot: `lampo status`, `lampo target <°C>`, `lampo history` (etuliite `lampo`, koska Zephyrillä on oma `history`). | Laitetta voi tutkia ja säätää ilman uudelleenkääntämistä. |
| F6 | Lokiin tärkeät tapahtumat: hälytys, anturivika, asetusmuutos. | Jälkikäteen nähdään, mitä laite teki. |
| F7 | Jos anturi ei vastaa → lämmitin pois ja hälytys. | Pahin tilanne on lämmitin jumissa päällä. Turvallisin tila on aina "lämmitin pois". |
| F8 | Fyysisesti mahdoton lukema (alle −40 °C, yli +85 °C tai epärealistinen hyppy) → käsitellään anturivikana kuten F7. | Anturi voi vastata mutta väärin, esim. jumittua näyttämään kylmää, jolloin ohjain lämmittäisi loputtomiin. F7 ei huomaa tätä. |
| F9 | Watchdog: jos ohjelma jumittuu, piiri käynnistyy uudelleen. Käynnistyksessä lämmitin on aina pois. | Oma ohjelmamme voi jumittua lämmitin päällä. Watchdog on toinen turvakerros. |

### Laatuvaatimukset

| # | Vaatimus | Miksi |
|---|---|---|
| N1 | Kaikki logiikka on testattavissa PC:llä ilman laitetta. | Huolimattomuusvirheet löytyvät sekunneissa eikä Wokwissa tai kentällä. |
| N2 | C-koodissa ei ole pinninumeroita; kaikki laitteet devicetreessä. | Levyn muutos koskee vain overlayta. |
| N3 | Jokaisella threadilla on kirjoitettu perustelu ja prioriteetti. | Thread maksaa pinon ja tuo synkronointia; jokaisen pitää ansaita paikkansa. |

### Ei tässä versiossa (ehkä myöhemmin)

OLED-näyttö, asetusten tallennus flashiin, min/max/keskiarvo, PID-säätö.

---

## 3. Päätökset

### P1. Alusta: Pico + Zephyr + Wokwi

- **Valittu:** Raspberry Pi Pico, Zephyr v4.5.0, Wokwi.
- **Miksi (rehellisesti):** tämä on **oppimisprojekti**. Tavoite on harjoitella Zephyriä; fyysistä
  rautaa ei ole, joten simulaattori on pakollinen; Wokwin STM32-emulaatio ei toimi RTOSin kanssa,
  joten piiriksi valikoitui Pico.
- **Oikeassa tuotteessa** tämän kokoinen termostaatti onnistuisi myös ilman RTOSia (super-loop,
  jossa anturin luku tehdään tilakoneena ilman odottamista) tai FreeRTOSilla. Zephyr olisi
  perusteltu, jos laitteeseen tulisi langattomuus (Bluetooth, verkko), jos samaa koodia
  ajettaisiin monella eri piirillä tai laudalla, tai jos elinkaari on pitkä.
- **Yleiset valintakriteerit** (vaatimuksista käsin):
  - *RTOS vai ei:* montako työtä eri tahdissa, voiko jokin jäädä odottamaan, tarvitaanko
    valmiita pinoja (BLE, TCP/IP, USB).
  - *FreeRTOS + valmistajan SDK vai Zephyr:* koko, yhden vai monen piirin tuki, langattomuus,
    tiimin osaaminen, oppimiskynnys.
  - *Piiri:* oheislaitteet, muisti, laskentateho, virrankulutus, hinta, saatavuus,
    käyttölämpötila, tiimin aiempi kokemus.
- **Toimivuus todennettu** 2026-11-18 Wokwissa (wokwi-cli 0.29.1):
  - Zephyr käynnistyy Picolla, ja monen threadin ohjelma (ajastin, semafori, workqueue,
    mutex) toimii: kurssin projekti `0097-threads-and-kernel`.
  - MYS-1 vastaa Picolla sekä Pico SDK:lla (lab 3) että Zephyrillä, kun väylänä on
    `gpio-i2c` (ks. P5).
- **Hylätty:** Blue Pill (STM32F103) Wokwissa. Wokwin STM32-emulaatiossa on kolme vikaa, jotka
  estävät RTOSin: NVIC:n prioriteettirekisterit puuttuvat, CPSIE/CPSID ovat toisin päin ja
  PendSV palaa väärään käskyyn (löydetty lab 5:ssä).

### P2. Lämmittimen ohjaus: on/off hystereesillä

- **Valittu:** kaksi rajaa, tavoite ± 0.5 °C.
- **Miksi:** yksinkertainen, helppo testata, ei kuluta relettä.
- **Hylätty:** yksi raja (rele kuluu, ks. F2). PID-säätö (tarkempi mutta monimutkaisempi; kurssin
  loppuprojekti käsittelee sen, ja on/off riittää terraarioon).
- **Kompromissi:** leveämpi kaista = harvemmat kytkennät mutta suurempi heilahtelu.

### P3. Vikatilanteessa lämmitin pois

- **Valittu:** jokainen vika (anturi ei vastaa, mahdoton lukema, ohjelman jumi) johtaa tilaan
  "lämmitin pois + hälytys".
- **Miksi:** jäähtyminen on hidasta ja korjattavissa, ylikuumeneminen ei.
- **Huomio:** oikeassa laitteessa olisi lisäksi raudallinen lämpösulake, koska ohjelmaan ei
  koskaan luoteta sataprosenttisesti (kerroksittainen turvallisuus).

### P4. Yksikkötestit PC:llä Unityllä

- **Valittu:** puhtaat logiikkamoduulit käännetään PC:llä gcc:llä ja testataan Unityllä
  (`third_party/Unity`). Tarvittaessa fff (`third_party/fff`) feikkaa funktiot.
- **Miksi:** toimii suoraan Windowsissa ja ajetaan sekunneissa.
- **Hylätty (toistaiseksi):** Zephyrin ztest + `native_sim`, koska se toimii vain Linuxissa.
  WSL (Ubuntu) on asennettu, joten voidaan ottaa myöhemmin käyttöön.
- **Testausperiaatteet:**
  - Testaa reunat: tasan rajalla, negatiiviset, suurimmat arvot.
  - Pieni syöteavaruus (esim. 16-bittinen raakaluku) → testaa kaikki syötteet.
  - Suuri syöteavaruus (esim. shell-komennon teksti) → satunnaistestaus kiinteällä siemenellä,
    ja jokainen löydetty bugi lisätään kiinteäksi testiksi.

### P5. I2C-väylä Wokwissa: ohjelmallinen `gpio-i2c`

- **Ongelma:** Zephyrin rautainen I2C-ajuri (DesignWare, `i2c0`) ei saa vastausta MYS-1:ltä
  Wokwissa: `error -116` (aikakatkaisu). Sama anturi samoissa pinneissä vastaa Pico SDK:lla.
- **Kokeiltu, ei auttanut:** väylänopeus 400 kHz → 100 kHz; sisäiset pull-upit pinctrl:ssä.
- **Todennäköinen syy:** Zephyrin ajuri odottaa I2C-keskeytyksiä, Pico SDK kyselee rekistereitä
  silmukassa. Wokwin RP2040-emulaatio ei ilmeisesti tuota näitä keskeytyksiä. Ei varmistettu.
- **Valittu:** Zephyrin `gpio-i2c`-ajuri, joka ohjaa pinnejä GP4/GP5 ohjelmallisesti, ja
  `i2c0` pois päältä. Testattu: `MYSTERY WHO_AM_I 0xa7`.
- **Miksi tämä on hyvä ratkaisu:** vaihto tehdään **pelkästään overlayssa**. C-koodi
  (`I2C_DT_SPEC_GET`, `i2c_write_read_dt`) on täsmälleen sama molemmilla väylillä. Oikealla
  Picolla palattaisiin rautaiseen `i2c0`:aan vaihtamalla vain overlay.
- **Hinta:** ohjelmallinen väylä kuluttaa CPU-aikaa ja on hitaampi. 1 Hz:n lukemiselle
  merkityksetöntä.

---

## 4. Arkkitehtuuri

### 4.1 Työt ja threadit (vaihe 2a, päätetty)

| Työ | Milloin | Kesto | Kuka tekee |
|---|---|---|---|
| Lue anturi | 1 s välein | voi odottaa jopa 2 s (DRDY + aikakatkaisu) | `sensor`-thread |
| Hystereesi, hälytyslogiikka, historia | lukeman / tapahtuman jälkeen | mikrosekunteja | `control`-thread |
| Napin painallus | milloin tahansa | keskeytys | GPIO-keskeytys → viesti `control`-threadille |
| Hälytys-LEDin vilkutus | 2 Hz hälytyksen aikana | mikrosekunteja | `k_timer` (ei threadia) |
| Shell, loki | tarvittaessa | ms | Zephyrin omat threadit |

**Valittu:** kaksi omaa threadia, ryhmiteltynä kahdella säännöllä:
1. **Mikä voi jäädä odottamaan, eristetään.** Anturin luku voi odottaa 2 s; jos se olisi samassa
   threadissa logiikan kanssa, napin kuittaus ja vikareaktiot viivästyisivät saman verran.
2. **Mikä käyttää samaa tietoa, samaan threadiin.** Hystereesi, hälytys ja historia käyttävät
   lämpötilaa, tavoitetta ja hälytystilaa. Kun yksi thread omistaa tiedon (**yksi kirjoittaja**),
   sen sisällä ei tarvita mutexeja.

**Hylätyt:**
- *Yksi thread (super-loop):* anturin 2 s odotus pysäyttäisi kaiken muun (lessonin 86 ongelma).
- *Oma thread jokaiselle työlle:* jaettu tieto vaatisi mutexin jokaisen muuttujan ympärille;
  enemmän lukkoja = enemmän deadlock- ja unohdusmahdollisuuksia.

### 4.2 Viestit ja jaettu tieto (vaihe 2b, päätetty)

**Periaate: `control` omistaa kaiken tilan, eikä kukaan muu kirjoita siihen.** Muut lähettävät
viestejä, ja `control` päättää itse, mitä tekee.

```
sensor-thread ──EV_SAMPLE / EV_SENSOR_FAULT──┐
nappi (keskeytys) ──EV_BUTTON────────────────┼──► postilaatikko (k_msgq) ──► control
shell ──EV_SET_TARGET────────────────────────┘
control ──► "valokuva" tilasta (mutexin takana) ──► shell lukee (status, history)
```

**Postilaatikko: yksi `k_msgq` kaikille tapahtumille.**
- **Miksi yksi:** thread voi odottaa kerrallaan vain yhtä jonoa. Yksi laatikko herättää
  `control`-threadin mistä tahansa syystä (sama ratkaisu kuin lab 5:n `ui_queue`).
- **Viesti:** rakenne, jossa on tyyppi (`EV_SAMPLE`, `EV_SENSOR_FAULT`, `EV_BUTTON`,
  `EV_SET_TARGET`) ja arvo (lämpötila tai tavoite sadasosina).

**Tavoitteen muutos viestinä, ei suorana kirjoituksena.**
- **Miksi:** suora kirjoitus rikkoisi "yksi kirjoittaja" -periaatteen, ja `control` ei tietäisi
  muutoksesta. Viesti herättää `control`-threadin, joka tarkistaa arvon (hylkää mahdottomat),
  kirjaa muutoksen lokiin (F6) ja päättää lämmittimestä heti uudelleen.

**Shellin lukeminen: "valokuva".**
- `control` kopioi tilansa valokuvaan jokaisen päivityksen jälkeen; shell kopioi valokuvan.
  Molemmat pitävät mutexia vain kopioinnin ajan (mikrosekunteja). Tulostus tehdään lukon
  ulkopuolella.
- **Koko järjestelmässä on yksi mutex**, ja se suojaa vain valokuvaa.
- **Hylätyt:** lukeminen ilman lukkoa (tila on monta muuttujaa → shell voisi nähdä puoliksi
  päivitetyn kuvan); kysymys–vastaus-viestit (toinen jono vastauksille, liian monimutkainen).

**Täysi postilaatikko.**
- Kaikki lähettäjät käyttävät `K_NO_WAIT`:ia: nappi koska keskeytys ei saa odottaa, sensor
  koska jumiin jäänyt sensor on pahempi kuin yksi kadonnut lukema (seuraava tulee 1 s päästä).
- Täysi laatikko on **oire** (`control` jumissa), ei normaali tilanne. Pudotukset lasketaan ja
  kirjataan lokiin, ja laskuri näkyy `status`-komennossa. Jos `control` on oikeasti jumissa,
  watchdog (F9) käynnistää laitteen uudelleen.
- **Hylätty:** "uusin arvo voittaa" (poista vanhin lukema laatikosta). Hyvä idea lämpötilalle,
  mutta vaatisi sensor-threadin lukemaan `control`-threadin laatikkoa → kaksi lukijaa.

### 4.3 Moduulijako ja testattavuus (vaihe 2c)

**Lähtöongelma:** suoraviivaisesti kirjoitetussa `control`-threadissa päätökset (hystereesi,
vikareaktio) ja tekeminen (`k_msgq_get`, `k_timer_start`, `LOG_ERR`, `gpio_pin_set_dt`) ovat
sekaisin. Zephyr-kutsut eivät toimi PC:llä, joten päätöksiä ei voi testata ilman laitetta (N1).

**Havainto:** kaikki *päättävät* rivit ovat tavallista C:tä; kaikki *tekevät* rivit koskevat
ulkomaailmaa (Zephyr, rauta). Jaetaan koodi niiden mukaan:

- **Ydin** (päättää, puhdasta C:tä, ei yhtään Zephyr-otsikkoa) → testataan PC:llä Unityllä.
- **Kuori** (tekee: odottaa viestejä, lukee anturia, sytyttää LEDejä, lokittaa) → testataan
  Wokwissa. Kuori on ohut ja "tyhmä": se välittää eikä päätä.

**Ytimen rajapinta, ja miksi se on tällainen:**

```c
struct actions decide(struct state *s, struct event ev, uint32_t now_ms);
```

| Valinta | Miksi |
|---|---|
| Palauttaa päätöksen (`return`) eikä toteuta sitä | Toteuttaminen vaatisi Zephyr-kutsuja → ei toimisi PC:llä. Postilaatikkokaan ei käy, sillä se on Zephyrin. |
| Päätökset `struct actions`:ssa (`heater_on`, `alarm_on`) | `return` palauttaa yhden asian; struct kokoaa monta. Uusi päätös = uusi kenttä. |
| Aika parametrina (`now_ms`) | `k_uptime_get_32()` ei toimi PC:llä. Testi voi antaa minkä tahansa ajan, joten "31 s myöhemmin" testataan millisekunnissa ilman odottamista. |
| Tila `struct state`:ssa, annetaan osoitteena | Tilaa on paljon (lämmittimen tila, tavoite, milloin karkasi, kuittaus, edellinen lukema, historia), ja muutosten pitää säilyä kutsusta toiseen. Osoite → muutetaan alkuperäistä, ei kopiota. |

**Kuori käyttää ydintä näin:**

```c
static struct state st = { .target = 2500 };
while (1) {
    k_msgq_get(&inbox, &ev, K_FOREVER);
    struct actions act = decide(&st, ev, k_uptime_get_32());
    gpio_pin_set_dt(&heater_led, act.heater_on);
    /* hälytys-LED, valokuva shellille, watchdog */
}
```

**Valvonta:** ydin käännetään testejä varten PC:n gcc:llä, jossa Zephyriä ei ole. Jos ytimeen
lisätään vahingossa Zephyr-kutsu, PC-käännös kaatuu heti.

**Hylätyt:**
- *Logiikka suoraan threadissa:* ei testattavissa ilman rautaa.
- *Ydin kutsuu `gpio_pin_set_dt()`:tä ja testissä se feikataan (fff):* toimii, mutta testi
  tarkistaa "kutsuttiinko oikeaa funktiota" eikä "mikä päätös tehtiin". Paluuarvo on
  suoraviivaisempi.

**Ydin kertoo myös syyn.** `struct actions` sisältää kentän `fault` (`FAULT_NONE`,
`FAULT_NO_ANSWER`, `FAULT_IMPOSSIBLE`). Ydin ei voi lokittaa itse (loki on Zephyrin), joten se
palauttaa syyn ja kuori kirjoittaa sen lokiin (F6).

**Ydin jaetaan pienempiin moduuleihin.**

- **Lähtöongelma:** jos `decide()` tekee kaiken (hystereesi, hälytys, uskottavuus, historia),
  testi "24.40 °C → lämmitin päälle" voi kaatua uskottavuustarkistuksen vian takia. Testin nimi
  osoittaa väärään paikkaan, ja vika pitää etsiä käsin.
- **Ratkaisu:** jokainen osa omaksi funktiokseen ja tiedostokseen, omine testeineen. Kun jokin
  kaatuu, testilista kertoo **poissulkemalla**, missä osassa vika on.

```
controller.c    decide()            kokoaa osat; testataan yhteistoiminta (esim. vika → lämmitin pois)
  plausibility.c  plaus_ok()        F8: mahdoton lukema / epärealistinen hyppy
  hysteresis.c    hyst_update()     F2: lämmitin päälle/pois
  alarm.c                           F3/F4: hälytyksen ajoitus ja kuittaus
  history.c                         F5: viimeiset lukemat
mys1_conv.c     raakaluku → sadasosa-asteet
cmd_parse.c     käyttäjän teksti → luku tai virhe
```

**Testausstrategia moduuleittain:**

| Moduuli | Tapa | Miksi |
|---|---|---|
| `mys1_conv.c` | **Kaikki syötteet** (65 536) verrattuna liukulukulaskuun + muutama kiinteä esimerkki | Syötteitä vähän → ei jätetä yhtään testaamatta. Kiinteät esimerkit toimivat dokumentaationa. |
| `hysteresis.c` | **Reunatapaukset**: tasan rajalla, ±1 sadasosa, molemmat lähtötilat, kaistan sisällä ei muutosta. Lisäksi ominaisuustesti satunnaisilla arvoilla: kaistan sisällä tila ei koskaan muutu. | Yhdistelmiä liikaa käydä kaikki läpi; virheet piiloutuvat rajoille (`<` vs. `<=`). |
| `plausibility.c`, `alarm.c`, `history.c` | Reunatapaukset (esim. hälytys 29.999 s / 30.000 s / 30.001 s) | Sama syy. |
| `cmd_parse.c` | **Satunnaiset syötteet kiinteällä siemenellä** + kiinteät tunnetut tapaukset | Ihminen voi kirjoittaa mitä tahansa → rajaton syöteavaruus. Jokainen löydetty bugi lisätään kiinteäksi testiksi. |
| `controller.c` | Tapahtumaketjut (esim. kylmä → lämmitin päälle → anturivika → lämmitin pois + hälytys) | Testaa, että osat toimivat yhdessä. |

### 4.4 Prioriteetit (vaihe 2d, päätetty)

Zephyrissä pienempi numero = kiireellisempi.

| Thread | Prioriteetti | Perustelu |
|---|---|---|
| `control` | `K_PRIO_PREEMPT(2)` | Lyhin määräaika: vikareaktio (lämmitin pois) heti, napin kuittaus alle 50 ms. Työ kestää mikrosekunteja, joten se viivästyttää muita vain hetken. |
| `sensor` | `K_PRIO_PREEMPT(4)` | 1 s jakso; muutaman millisekunnin viive ei haittaa, koska lämpötila muuttuu hitaasti. Käyttää CPU:ta koko I2C-luvun ajan (`gpio-i2c`, P5), mutta luku kestää noin 1 ms. |
| shell, loki | Zephyrin oletus: alin sovellusprioriteetti | Ihminen ei huomaa alle 100 ms:n viivettä. |

- **Sääntö:** *deadline-monotonic*, eli lyhin määräaika saa korkeimman prioriteetin. `control`-threadilla
  ei ole jaksoa (herää tapahtumista), joten pelkkä rate-monotonic ei suoraan päde.
- **Väli numeroissa** (2 ja 4): tilaa lisätä thread väliin myöhemmin.
- **Ansa, tarkistettu lähteestä:** `CONFIG_SHELL_THREAD_PRIORITY` on oletuksena 0, mikä näyttäisi
  nostavan shellin `control`-threadin yli. Arvoa käytetään kuitenkin vain, jos
  `CONFIG_SHELL_THREAD_PRIORITY_OVERRIDE=y`; muuten shell saa alimman sovellusprioriteetin
  (`subsys/shell/shell.c`, rivi 42). Lokithread samoin (`subsys/logging/log_core.c`, rivi 63).
- **Hylätty:** `sensor` korkeammalle (rate-monotonic jakson mukaan). Silloin vikareaktio voisi
  joutua odottamaan anturin luvun loppuun.

### 4.5 Watchdog ja hiljaisen anturin tunnistus (vaihe 2d, päätetty)

**Periaate: ruokkijan pitää olla se, jonka toiminta halutaan todistaa.**

**Valittu: kaksi valvontakerrosta.**

| Kuka valvoo | Ketä | Miten |
|---|---|---|
| `control` | `sensor`-threadia | Muistaa viimeisen lukeman ajan (`last_sample_ms` tilassa). Jokaisella heräämisellä: jos yli 2.5 s ilman lukemaa → anturivika (lämmitin pois, hälytys, loki). Laite pysyy käynnissä turvatilassa, ja vikaa voi tutkia shellillä. |
| watchdog (rauta) | `control`-threadia | `control` ruokkii watchdogia jokaisen silmukkakierroksen lopussa. Jos `control` jumittuu (pyörii tai odottaa ikuisesti), ruokinta loppuu ja piiri käynnistyy uudelleen lämmitin pois päältä (F9). |

**Toteutuksen yksityiskohdat:**
- `control` odottaa postilaatikkoa enintään 2.5 s (`K_MSEC(2500)`, ei `K_FOREVER`). Näin se
  herää vähintään 2.5 s välein, vaikka postia ei tulisi, ja ehtii tehdä tarkistuksen ja ruokkia.
- Tarkistus tehdään **ytimessä** (`decide()`, aika parametrina) → testattavissa PC:llä, esim.
  "lukema hetkellä 0, nappi hetkillä 1000, 2000, 3000 → hetkellä 3000 anturivika".
- Ajan vertailu wrap-turvallisesti: `(uint32_t)(now_ms - s->last_sample_ms) > 2500`.

**Hylätyt:**
- *Erillinen feeder-thread, joka ruokkii 1 s välein:* todistaa vain, että feeder itse toimii.
  Jos `control` jää odottamaan ikuisesti (esim. mutexia), feeder saa vuoron ja ruokkii, eikä
  watchdog laukea koskaan. Lämmitin voi jäädä päälle.
- *`control` ruokkii, mutta odottaa postia `K_FOREVER`:lla:* turvallinen (watchdog käynnistää
  laitteen uudelleen 3 s päästä), mutta jos anturi jumittuu toistuvasti, laite jää
  uudelleenkäynnistyssilmukkaan ilman näkyvää hälytystä ja lokia.
- *Pelkkä postilaatikon aikakatkaisu ilman `last_sample_ms`-tarkistusta:* napin painallukset ja
  shell-komennot herättävät `control`-threadin, jolloin aikakatkaisu ei koskaan laukea, vaikka
  anturi on hiljaa.

**Avoin:** tukeeko Wokwi RP2040:n watchdogia? Testataan ennen toteutusta (ks. avoimet kysymykset).

### 4.6 Arkkitehtuuri yhdellä silmäyksellä

```
                 ┌──────────────┐  EV_SAMPLE / EV_SENSOR_FAULT
   MYS-1 ──I2C──►│ sensor (4)   │──────────────┐
  (gpio-i2c)     └──────────────┘              │
                                               ▼
   nappi ──keskeytys──EV_BUTTON──────► postilaatikko (k_msgq) ──► ┌──────────────────┐
                                               ▲                  │ control (2)      │──► lämmitin-LED
   shell (14) ──EV_SET_TARGET──────────────────┘                  │ kuori: odota,    │──► hälytys (k_timer)
       ▲                                                          │ toteuta, ruoki   │──► watchdog
       └────────── valokuva (mutex) ◄─────────────────────────────│ ydin: decide()   │──► loki (14)
                                                                  └──────────────────┘
```

Tila: arkkitehtuuri valmis 2026-11-18.

---

## 5. Toteutusjärjestys (vaihe 3, päätetty)

**Periaatteet:**
1. **Ensin se, millä näkee, toimiiko mikään:** runko Wokwissa ja testiympäristö PC:llä.
2. **Epävarmat asiat ensin:** jos jokin ei toimi lainkaan, se halutaan tietää ennen kuin sen
   varaan on rakennettu (sama kuin I2C-kokeilu, P5). Kokeilu ≠ toteutus.
3. **Testit ennen koodia** ytimen moduuleissa.
4. **Kävelevä luuranko:** koko ketju anturista LEDiin ensin ohuena (`decide()` tynkänä), sitten
   oikeat moduulit yksi kerrallaan tyngän tilalle.
5. **Jokaisen vaiheen lopussa toimiva versio.** Jos jokin hajoaa, syy on viimeisimmässä
   vaiheessa (poissulkeminen).

| Vaihe | Mitä | Valmis, kun |
|---|---|---|
| 0 | Kokeilut: watchdog Wokwissa, ajoituksen tarkkuus | Tiedetään, toimiiko watchdog ja kuinka tarkka aika on |
| 1 | Runko (`CMakeLists.txt`, `prj.conf`, overlay, `main.c`) + loki | "hei" ja lokirivi Wokwin sarjaportissa |
| 2 | PC-testiympäristö (Unity + gcc) | Yksi testi menee läpi yhdellä komennolla |
| 3 | `sensor`-thread + MYS-1 (`mys1_conv` testit edellä) | Lämpötila lokissa Wokwissa |
| 4 | `control`-silmukka tyngällä + lämmitin-LED | Luuranko toimii päästä päähän |
| 5 | Ydinmoduulit testit edellä, yksi kerrallaan | PC-testit vihreinä, tynkä korvattu |
| 6 | Nappi ja keskeytys | Hälytyksen kuittaus toimii Wokwissa |
| 7 | Shell + `cmd_parse` (satunnaistesti) | Komennot toimivat Wokwissa |
| 8 | Watchdog | Tahallinen jumi → uudelleenkäynnistys, lämmitin pois |

**Kansiorakenne:**

```
lampovahti/          oma git-repo (siirretty kurssin reposta 2026-11-18)
├── DESIGN.md
├── README.md
├── firmware/        Zephyr-sovellus = kuori (alun perin kurssin zephyr-pico-pohjasta)
├── core/            ydinmoduulit, puhdasta C:tä (vaihe 3 alkaen)
├── tests/           PC-testit Unityllä (vaihe 2)
├── spikes/          kertakäyttöiset kokeilut (vaihe 0)
├── scripts/         build.ps1 (west build), sim.ps1 (wokwi-cli)
├── diagram.json     Wokwin kytkennät
└── wokwi.toml       Wokwin asetukset
```

- `core/` on `firmware/`-kansion ulkopuolella, koska sitä käyttää kaksi käännöstä (Zephyr-firmware
  ja PC-testit), ja ytimen kuuluu olla Zephyristä riippumaton.
- **Oma repo eikä kurssin repon fork:** portfoliossa projekti näkyy heti etusivulla, ja historia
  sisältää vain tämän projektin.
- **Itsenäinen käännös:** `scripts/build.ps1` asettaa Zephyrin ympäristömuuttujat ja kutsuu
  `west build` (tämän kurssin `tools/build.py` teki aiemmin). Polut ovat oletuksia, jotka voi
  ohittaa ympäristömuuttujilla, joten repon voi kloonata ja kääntää ilman kurssin kansiota.

**Edistyminen:**

| Vaihe | Tila | Todennettu |
|---|---|---|
| 0 | ✓ 2026-11-18 | ks. avoimet kysymykset (watchdog, ajoitus) |
| 1 | ✓ 2026-11-18 | Wokwi: `<inf> main: Lämpövahdin runko käynnistyi`; debug-rivi ei näy (taso info) |
| 2 | ✓ 2026-11-18 | `scripts/test.ps1`: `100% tests passed out of 1`; testi rikottu tahallaan → `Expected 4 Was 5` |
| 3 | ✓ 2026-11-18 | `mys1_conv` TDD:llä (6 testiä + kaikki 65 536 syötettä). Anturi kytketty (WHO_AM_I 0xa7). Sensor-thread lukee lämpötilan lokiin Wokwissa yli 4 minuutin ajon ilman yhtään anturivikaa (`T = 22.59 C` ... MYS-1:n 8 arvon sarja toistuu). |
| 4 | ✓ 2026-11-18 | `decide()`-tynkä + 4 PC-testiä; rajatapaustesti kaatuu, kun `<` vaihdetaan `<=`:ksi. Wokwissa anturi → postilaatikko → control → LED: `T = 22.59 C` → `lammitin PAALLE`, `T = 22.70 C` → `lammitin POIS`. LED räpsyy tavoitteen ympärillä (ei hystereesiä, tarkoituksella). |
| 5 | ✓ 2026-11-18 | Ydin TDD:llä: hystereesi, uskottavuus, hälytys (myös laskurin pyörähdys), historia; jokainen testi nähty kaatuvan. Wokwissa lämmitin vaihtaa vain kaistan reunoilla (22.50 päälle, 22.93 pois); hälytys todettu tilapäisellä ± 0.10 °C:n alueella. Historia näyttää vain viimeiset 32 tapahtumaa: harvinaisten tapahtumien oma rengas on myöhempi parannus. |
| 6 | ✓ 2026-11-18 | Nappi GP13 (aktiivinen alhaalla, sisäinen pull-up). ISR vain ajastaa työn; värähtely suodatetaan työjonossa (k_work_reschedule 30 ms, pinni luetaan uudelleen). decide() kuittaa ilman alarm_update()-kutsua: muuten oletus ALARM_IN_FAULT hälytti napista (testi löysi). Wokwissa painallus kuittasi hälytyksen ja se palasi torkun jälkeen. |
| 7 | ✓ 2026-11-19 | Komennot `lampo status/target/history`. cmd_parse TDD:llä: taulukko + 10 000 edestakaista + 100 000 roskasyötettä kiinteillä siemenillä. Ylivuotosuojan testi löytyi vasta mutaatiolla: "99999999999" hylättiin vahingossa, joten lisättiin "4294967319" (2^32 + 23 → olisi 23.00 °C). Tavoitteen vaihto ei aja hystereesiä: prev_centi voi olla hylätty lukema. Valokuva yhden mutexin takana. Wokwissa kaikki komennot toimivat. |

**Päätös: ajat sovelluksen omina Kconfig-symboleina** (vaihe 3, `firmware/Kconfig`)
- **Miksi:** Wokwissa Zephyrin aika ja simuloitu anturi eivät kulje samaa tahtia (avoimet
  kysymykset), joten oikean raudan ajat eivät toimi simulaattorissa. Kconfigissa oletusarvo on
  oikean raudan arvo, ja `prj.conf` ohittaa sen Wokwille. Lopullinen arvo näkyy `.config`:ssa.
- **Arvot:** `CONFIG_LAMPO_SENSOR_PERIOD_MS` = 1000 (oletus). `CONFIG_LAMPO_DRDY_TIMEOUT_MS`:
  oletus 2000 (datalehti: näyte 500 ms välein, aikakatkaisu 4×), Wokwi **15000** (mitattu väli
  noin 7000 ms, ensimmäinen näyte 7771 ms → noin 2× marginaali). 9000 harkittiin: nopeampi
  vikailmoitus, mutta vain noin 16 % marginaali ensimmäiseen näytteeseen.
- **Hylätty:** `#define` otsikkotiedostossa (arvon muutos vaatisi koodin muokkausta, eikä eri
  ympäristöille voisi antaa eri arvoja ilman `#ifdef`-viidakkoa).
- **Myöhemmin:** samat periaatteet `control`-threadin aikoihin (hiljaisuusraja, hälytysviive).
  Ydin saa ne parametreina, koska ydin ei saa riippua Zephyrin Kconfigista.

**Opittu (vaihe 3):** uusi overlay- tai binding-tiedosto vaatii **puhtaan käännöksen**
(`scripts/build.ps1 -Clean`). Zephyr päättää mukaan otettavat tiedostot vain build-kansiota
luodessaan; vanhassa buildissa uusi overlay jäi lukematta ja käännös kaatui virheeseen
`DT_N_NODELABEL_mys1 ... undeclared`. Lisäksi epäonnistuneen käännöksen jälkeen simulaattori ajaa
edellistä `zephyr.elf`-tiedostoa, joten tuloste on vanha.

**Päätös: Unity haetaan käännösvaiheessa (CMake FetchContent)** (vaihe 2)
- **Miksi:** kloonaus ei vaadi lisäaskeleita, eikä repoon tule vierasta koodia.
- **Hinta:** ensimmäinen (ja puhdas) käännös tarvitsee verkkoyhteyden; ladattu kopio jää
  `tests/build/_deps`-kansioon.
- **Lukitus:** commitin tarkistesummaan (`cbcd08f…` = v2.6.1), ei versionimeen. Versionimen voi
  siirtää osoittamaan toiseen committiin, tarkistesummaa ei.
- **Hylätyt:** tiedostojen kopiointi repoon (päivitys käsin, vierasta koodia historiassa, lisenssi
  mukaan); git submodule (kloonaus vaatii `--recursive`, unohtuessa sekava käännösvirhe).
- **Lisäksi:** testit käännetään `-Werror`:lla, joten varoitukset pysäyttävät käännöksen.

**Päätös: lokitila `CONFIG_LOG_MODE_DEFERRED`** (vaihe 1)
- **Miksi:** `LOG_*` vain laittaa viestin puskuriin ja palaa heti; Zephyrin lokithread (alin
  prioriteetti) tulostaa sen myöhemmin. Muuten `control`-thread odottaisi hidasta UARTia
  jokaisen lokirivin kohdalla, ja vikareaktio hidastuisi. Sama periaate kuin koko
  suunnitelmassa: hidas työ pois kiireellisestä threadista.
- **Hinta:** puskurissa odottavat rivit katoavat, jos laite käynnistyy uudelleen ennen
  tulostusta (watchdog, virtakatko). Fatal error -tilanteessa Zephyr tyhjentää puskurin ennen
  pysähtymistä. Jos rivejä tulee kerralla enemmän kuin puskuriin mahtuu, osa pudotetaan.
- **Hylätty:** `CONFIG_LOG_MODE_IMMEDIATE` (kutsuja odottaa UARTia).

**Muutokset ensimmäiseen ehdotukseen ja miksi:**
- *Watchdogin kokeilu siirretty alkuun (vaihe 0):* se on projektin epävarmin asia. Varsinainen
  toteutus pysyy lopussa.
- *Loki siirretty runkoon (vaihe 1):* lokia tarvitaan koko kehityksen ajan vikojen etsimiseen.

---

## 6. Avoimet kysymykset

| Kysymys | Ehdotus | Tila |
|---|---|---|
| F3: mikä on "sallittu alue" ja "liian kauan"? | Tavoite ± 2 °C, yhtäjaksoisesti 30 s | Ehdotus, päätetään vaiheessa 2 |
| Voiko MYS-1:n lämpötilaa muuttaa Wokwissa hälytysten testaamiseksi? | Lämpötila vaihtelee itsestään hieman (22.6–22.9 °C lab 3:ssa). Suurta muutosta ei vielä osata tehdä; hälytykset voi testata yksikkötesteillä. | Osittain selvitetty |
| Tukeeko Wokwi RP2040:n watchdogia (F9)? | **Osittain.** Kokeilu `spikes/watchdog` (2026-11-18): watchdog käynnistyy (CTRL-rekisterin ENABLE = 1) ja laskuri TIME laskee nollaan ja pyörähtää ympäri, mutta **piiri ei käynnisty uudelleen**. Koodimme on siis oikein; Wokwi ei emuloi uudelleenkäynnistystä. Vaihtoehdot vaiheeseen 8: raudallinen watchdog suunnitelman mukaan (toimisi oikealla Picolla, ei testattavissa Wokwissa) ja/tai Zephyrin task watchdog + ohjelmallinen uudelleenkäynnistys (pitää kokeilla, toimiiko Wokwissa). | Selvitetty, päätös vaiheessa 8 |
| Kulkeeko Zephyrin aika samaa tahtia kuin simuloitu anturi? | **Ei.** Kokeilu vaiheessa 3 (kopio, ei commitoitu): MYS-1 tuottaa näytteen 500 ms välein simulaattorin ajassa (datalehti ja piirin lähdekoodi), mutta Zephyrin `k_uptime_get()` näyttää DRDY:n nousevan noin **7 s** välein. Lukemat tulevat järjestyksessä, joten näytteitä ei hukata. Zephyrin kello näyttää käyvän Wokwissa monta kertaa nopeammin kuin simuloidut oheislaitteet. Lab 3 (Pico SDK, ei Zephyr) lukee samaa anturia oikeassa tahdissa. **Vaikutus:** suunnitelman ajat (1 s jakso, 2.5 s hiljaisuus, 30 s hälytys) eivät päde Wokwissa sellaisinaan. | Avoin: päätös vaiheessa 3 |
| Onko Zephyrin ajoitus Wokwin Picolla tarkka? | **Ei täysin.** Kokeilu `spikes/watchdog`: `k_msleep(1000)` kestää 1154 ms ja `k_msleep(250)` 404 ms, eli uni venyy noin **154 ms** kestosta riippumatta. Mitattu ilman printk:ta mittausvälissä, joten hypoteesi "printk vie ajan" kumoutui. **Vaikutus:** `decide()` vertaa aikaleimoja (`now_ms`), joten hälytyksen 30 s ja anturin 2.5 s hiljaisuus mitataan oikein. Sensorin jakso venyy 1.15 sekuntiin, mikä ei haittaa; vaiheessa 3 jakso tehdään ajastimella tai absoluuttisella ajalla, jotta viive ei kerry. | Selvitetty |
