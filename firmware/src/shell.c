/* shell.c: shell-komennot (F5, DESIGN.md vaihe 7). Kuorta: teksti sisään, teksti ulos.
 *
 *   lampo status            viimeisin lukema, tavoite, lämmitin, hälytys
 *   lampo target <°C>       uusi tavoite, esim. "lampo target 23.5" tai "lampo target 23,5"
 *   lampo history           viimeiset tapahtumat vanhimmasta uusimpaan
 *
 * Komennot ovat lampo-pääkomennon alla, koska Zephyrin shellissä on jo oma "history"
 * (komentohistoria). Oma etuliite pitää ne erillään Zephyrin komennoista.
 *
 * Shell EI muuta tilaa itse: tavoite lähetetään postilaatikkoon (EV_SET_TARGET), ja
 * control-thread on yhä tilan ainoa kirjoittaja. Lukeminen tehdään valokuvasta.
 */
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "cmd_parse.h"
#include "control.h"

/* Valokuvan kopio. static eikä pinossa: noin 0.5 kt, ja shell-threadin pino on 2 kt.
 * Komennot ajetaan aina shell-threadissa yksi kerrallaan, joten yksi puskuri riittää. */
static struct state view;

/* Sadasosat tekstiksi: -5 → "-0.05". Pelkkä %d.%02d antaisi "0.-5". */
static const char *centi_str(int32_t c, char buf[12])
{
    int32_t a = (c < 0) ? -c : c;
    snprintf(buf, 12, "%s%d.%02d", (c < 0) ? "-" : "", (int)(a / 100), (int)(a % 100));
    return buf;
}

static const char *kind_str(enum history_kind k)
{
    switch (k) {
    case HIST_OK:       return "ok";
    case HIST_REJECTED: return "HYLATTY";
    case HIST_FAULT:    return "VIKA";
    }
    return "?";
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    char a[12], b[12];

    control_snapshot(&view);

    size_t n = history_count(&view.history);
    if (n == 0) {
        shell_print(sh, "lukema:    (ei viela)");
    } else {
        struct history_entry e = history_get(&view.history, n - 1);
        if (e.kind == HIST_FAULT) {
            shell_print(sh, "lukema:    VIKA (anturi ei vastaa)");
        } else {
            shell_print(sh, "lukema:    %s C (%s)", centi_str(e.centi, a), kind_str(e.kind));
        }
    }
    shell_print(sh, "tavoite:   %s C (kaista %s C)", centi_str(view.target_centi, a),
                centi_str(view.band_centi, b));
    shell_print(sh, "lammitin:  %s", view.heater_on ? "PAALLA" : "pois");
    shell_print(sh, "halytys:   %s", view.alarm.on ? "PAALLA" : (view.alarm.snoozed ? "kuitattu" : "ei"));
    return 0;
}

static int cmd_target(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);       /* shell tarkistaa: tasan yksi argumentti (SHELL_CMD_ARG alla) */
    char a[12], b[12];
    int32_t centi;

    switch (parse_target_centi(argv[1], &centi)) {
    case PARSE_OK:
        break;
    case PARSE_EMPTY:
    case PARSE_SYNTAX:
        shell_error(sh, "'%s' ei ole lampotila. Esimerkki: lampo target 23.5", argv[1]);
        return -EINVAL;
    case PARSE_RANGE:
        shell_error(sh, "sallittu alue %s ... %s C", centi_str(TARGET_MIN_CENTI, a),
                    centi_str(TARGET_MAX_CENTI, b));
        return -EINVAL;
    }

    struct event ev = { .type = EV_SET_TARGET, .centi = centi };
    if (control_post(ev) != 0) {
        shell_error(sh, "postilaatikko taynna, yrita uudelleen");
        return -EBUSY;
    }
    shell_print(sh, "tavoite -> %s C (voimaan seuraavasta lukemasta)", centi_str(centi, a));
    return 0;
}

static int cmd_history(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    char a[12];

    control_snapshot(&view);

    size_t n = history_count(&view.history);
    if (n == 0) {
        shell_print(sh, "(tyhja)");
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        struct history_entry e = history_get(&view.history, i);
        if (e.kind == HIST_FAULT) {
            shell_print(sh, "%6u.%03u s        VIKA", e.t_ms / 1000, e.t_ms % 1000);
        } else {
            shell_print(sh, "%6u.%03u s  %7s  %s", e.t_ms / 1000, e.t_ms % 1000,
                        centi_str(e.centi, a), kind_str(e.kind));
        }
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_lampo,
    SHELL_CMD(status, NULL, "Lukema, tavoite, lammitin ja halytys", cmd_status),
    SHELL_CMD_ARG(target, NULL, "Aseta tavoite: lampo target <C>, esim. 23.5", cmd_target, 2, 0),
    SHELL_CMD(history, NULL, "Viimeiset tapahtumat (vanhin ensin)", cmd_history),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lampo, &sub_lampo, "Lampovahdin komennot", NULL);
