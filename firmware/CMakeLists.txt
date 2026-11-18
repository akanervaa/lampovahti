cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lampovahti)

# Kuori (firmware/src): Zephyr- ja rautakoodi.
target_sources(app PRIVATE
    src/main.c
    src/mys1.c
    src/sensor.c
    src/control.c
)

# Ydin (core/): sama puhdas C, joka testataan PC:llä (tests/). Tässä se käännetään Arm-kääntäjällä.
target_sources(app PRIVATE
    ../core/mys1_conv.c
    ../core/controller.c
    ../core/hysteresis.c
    ../core/plausibility.c
    ../core/alarm.c
)
target_include_directories(app PRIVATE ../core)
