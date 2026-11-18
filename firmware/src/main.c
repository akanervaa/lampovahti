/* main.c: Lämpövahdin käynnistys.
 *
 * Vaihe 1 (runko): todistetaan, että projekti kääntyy, käynnistyy Wokwissa ja loki toimii.
 * Myöhemmissä vaiheissa tänne tulee laitteiden alustus ja threadien käynnistys.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* (4) Rekisteröi tämä tiedosto lokimoduuliksi nimellä "main" ja tasolla info.
 *     Makro on LOG_MODULE_REGISTER(nimi, taso). Tasot: LOG_LEVEL_ERR, _WRN, _INF, _DBG.
 *     Mieti: miksi jokainen tiedosto saa oman nimen lokissa? (Vihje: kun lokissa on satoja
 *     rivejä kymmenestä tiedostosta, miten löydät sensorin rivit?) */
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    /* (5) Info-tason lokirivi, joka kertoo, että laite käynnistyi.
     *     Makro on LOG_INF("teksti"). */
    LOG_INF("Lämpövahdin runko käynnistyi");

    /* Tämä rivi EI saa näkyä, koska moduulin taso on info. Jos näet sen, jokin taso on väärin. */
    LOG_DBG("debug-rivi: ei pitaisi nakya");

    return 0;
}
