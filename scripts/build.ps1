<#
.SYNOPSIS
  Kääntää Zephyr-sovelluksen Raspberry Pi Picolle.

.DESCRIPTION
  Tämä on se, minkä kurssin tools/build.py teki puolestamme: asettaa Zephyrin tarvitsemat
  ympäristömuuttujat ja kutsuu `west build`. Polut ovat oletuksia, ja jokaisen voi ohittaa
  ympäristömuuttujalla, jos kone on asennettu eri tavalla.

    ZEPHYR_WORKSPACE           Zephyr-työtila (oletus: ~/zephyrproject, Zephyr v4.5.0)
    GNUARMEMB_TOOLCHAIN_PATH   Arm GNU -työkaluketjun juurikansio (se, jonka alla on bin/)

.EXAMPLE
  ./scripts/build.ps1                          # kääntää firmware/
  ./scripts/build.ps1 -Clean                   # poistaa build-kansion ensin
  ./scripts/build.ps1 -App spikes/watchdog     # kääntää kokeilun
#>
param(
    [string]$App = "firmware",
    [switch]$Clean
)
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$AppDir = if ([System.IO.Path]::IsPathRooted($App)) { $App } else { Join-Path $Root $App }
$BuildDir = Join-Path $AppDir "build"

# --- Ympäristö ------------------------------------------------------------------------------
$ZephyrWs = if ($env:ZEPHYR_WORKSPACE) { $env:ZEPHYR_WORKSPACE } else { Join-Path $HOME "zephyrproject" }
$West = Join-Path $ZephyrWs ".venv\Scripts\west.exe"

# Zephyr-sovellus on työtilan ulkopuolella ("freestanding application"), joten Zephyrille
# kerrotaan, missä sen lähdekoodi on.
$env:ZEPHYR_BASE = Join-Path $ZephyrWs "zephyr"

# Käytetään koneelle jo asennettua Arm GNU -työkaluketjua Zephyr SDK:n sijaan (lesson 92).
$env:ZEPHYR_TOOLCHAIN_VARIANT = "gnuarmemb"
if (-not $env:GNUARMEMB_TOOLCHAIN_PATH) {
    $env:GNUARMEMB_TOOLCHAIN_PATH = "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1"
}

# --- Tarkistukset: kerro selvästi, mikä puuttuu (fail fast) ----------------------------------
if (-not (Test-Path $West)) {
    Write-Error "west ei löydy: $West. Onko Zephyr-työtila asennettu? (ZEPHYR_WORKSPACE)"
}
if (-not (Test-Path $env:GNUARMEMB_TOOLCHAIN_PATH)) {
    Write-Error "Arm GNU -työkaluketjua ei löydy: $env:GNUARMEMB_TOOLCHAIN_PATH (GNUARMEMB_TOOLCHAIN_PATH)"
}
if (-not (Test-Path (Join-Path $AppDir "CMakeLists.txt"))) {
    Write-Error "Ei Zephyr-sovellus (CMakeLists.txt puuttuu): $AppDir"
}

# --- Käännös ---------------------------------------------------------------------------------
if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Recurse -Force $BuildDir
}

# west etsii työtilansa nykyisestä kansiosta ylöspäin, joten ajetaan se työtilan sisältä.
Push-Location $ZephyrWs
try {
    & $West build -p auto -b rpi_pico -d $BuildDir $AppDir
    $code = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $code
