/* main.c: Lämpövahdin käynnistys.
 *
 * Threadit käynnistyvät itse (K_THREAD_DEFINE), joten main vain ilmoittaa käynnistyksestä.
 * WHO_AM_I-tarkistus siirtyi anturin ajuriin (mys1.c, mys1_init), koska se kuuluu anturille.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Lampovahti kaynnistyy");
    return 0;
}
