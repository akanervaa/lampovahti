<#
.SYNOPSIS
  Kääntää ja ajaa ytimen yksikkötestit PC:llä.

.DESCRIPTION
  Käyttää PC:n omaa gcc:tä (ei Arm-kääntäjää eikä Zephyriä), CMakea ja Ninjaa.
  Ensimmäinen ajo lataa Unity-testikehyksen GitHubista (tests/CMakeLists.txt).

.EXAMPLE
  ./scripts/test.ps1           # käännä ja aja kaikki testit
  ./scripts/test.ps1 -Clean    # poista build-kansio ensin
#>
param(
    [switch]$Clean
)
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Src = Join-Path $Root "tests"
$Build = Join-Path $Src "build"

if ($Clean -and (Test-Path $Build)) {
    Remove-Item -Recurse -Force $Build
}

# Konfiguroi (vain jos build-kansiota ei vielä ole), käännä, aja.
if (-not (Test-Path (Join-Path $Build "CMakeCache.txt"))) {
    cmake -S $Src -B $Build -G Ninja -DCMAKE_C_COMPILER=gcc
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
cmake --build $Build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# --output-on-failure: kaatuneen testin tuloste näytetään, onnistuneiden ei.
ctest --test-dir $Build --output-on-failure
exit $LASTEXITCODE
