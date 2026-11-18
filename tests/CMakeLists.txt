# tests/CMakeLists.txt: ytimen yksikkötestit PC:llä (DESIGN.md P4, vaihe 2).
#
# Käännetään PC:n omalla gcc:llä, EI Arm-kääntäjällä eikä Zephyrillä. Siksi ydin (core/) ei
# saa sisältää yhtään Zephyr-kutsua: jos sisältää, tämä käännös kaatuu (DESIGN.md 4.3).
cmake_minimum_required(VERSION 3.24)
project(lampovahti_tests C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Varoitukset virheiksi: huolimattomuusvirheet (käyttämätön muuttuja, väärä tyyppi) pysäyttävät
# käännöksen eivätkä jää huomaamatta.
add_compile_options(-Wall -Wextra -Werror)

# --- Unity (päätös: haetaan käännösvaiheessa, CMake FetchContent) ---------------------------
# Lukitaan commitin tarkistesummaan eikä pelkkään versionimeen (tagiin): tagin voi siirtää
# osoittamaan toiseen committiin, tarkistesummaa ei. cbcd08f... = Unity v2.6.1.
# Ladattu kopio tallentuu build-kansioon (_deps), joten verkkoa tarvitaan vain ensimmäisessä
# (tai puhtaassa) käännöksessä.
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        cbcd08fa7de711053a3deec6339ee89cad5d2697   # v2.6.1
)
FetchContent_MakeAvailable(unity)

enable_testing()

# --- Testit ----------------------------------------------------------------------------------
# Putkitesti: todistaa, että kääntäjä, Unity ja testien ajo toimivat, ennen kuin oikeita
# testejä kirjoitetaan. Poistetaan, kun ensimmäinen oikea testi on olemassa.
add_executable(test_smoke test_smoke.c)
target_link_libraries(test_smoke PRIVATE unity)
add_test(NAME smoke COMMAND test_smoke)
