# Lämpövahti

Pieni lämpötilaa ohjaava laite (terraario, akvaario): mittaa lämpötilaa, ohjaa lämmitintä
hystereesillä ja hälyttää, jos jokin menee vikaan. Zephyr RTOS, Raspberry Pi Pico, ajetaan
Wokwi-simulaattorissa.

Suunnittelu ja kaikki päätökset perusteluineen: **[DESIGN.md](DESIGN.md)**.

## Tarvitaan

| Työkalu | Versio | Huomio |
|---|---|---|
| Zephyr-työtila | v4.5.0 | oletuspolku `~/zephyrproject` (muuta: `ZEPHYR_WORKSPACE`) |
| Arm GNU Toolchain | 14.2 | oletuspolku `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1` (muuta: `GNUARMEMB_TOOLCHAIN_PATH`) |
| CMake, Ninja | | PATH:ssa |
| Wokwi for VS Code | | simulaattori |
| Wokwi CLI (valinnainen) | 0.29 | komentoriviltä ajoon, tarvitsee `WOKWI_CLI_TOKEN` |

## Käyttö

Avaa tämä kansio VS Codessa.

| Mitä | VS Code | Komentorivi (PowerShell) |
|---|---|---|
| Käännä | `Ctrl+Shift+B` | `./scripts/build.ps1` |
| Puhdas käännös | Tasks: *Clean build firmware* | `./scripts/build.ps1 -Clean` |
| Simuloi | `F1` → *Wokwi: Start Simulator* | `./scripts/sim.ps1` |
| Yksikkötestit (PC) | `Ctrl+Shift+P` → *Tasks: Run Test Task* | `./scripts/test.ps1` |
| Debuggaa | *Wokwi: Start Simulator and Wait for Debugger*, sitten `F5` | |

## Rakenne

```
DESIGN.md        vaatimukset, arkkitehtuuri, päätökset
firmware/        Zephyr-sovellus (kuori)
core/            ydinmoduulit, puhdasta C:tä (tulossa)
tests/           PC-testit Unityllä (haetaan ensimmäisellä käännöksellä, tarvitsee verkon)
spikes/          kertakäyttöiset kokeilut
scripts/         käännös- ja simulointiskriptit
diagram.json     Wokwin kytkennät
wokwi.toml       Wokwin asetukset
```
