<#
.SYNOPSIS
  Ajaa käännetyn firmwaren Wokwi-simulaattorissa komentoriviltä ja tulostaa sarjaportin.

.DESCRIPTION
  VS Codessa simulaattori käynnistetään komennolla "Wokwi: Start Simulator". Tämä skripti on
  automaattista ajoa varten (esim. tarkistus ennen committia).

  wokwi.toml sisältää gdbServerPort-rivin VS Coden debuggausta varten, ja silloin wokwi-cli jää
  odottamaan debuggeria. Siksi skripti tekee build/sim-kansioon kopion, jossa porttia ei ole.

  Tarvitsee Wokwi CLI:n (~/.wokwi/bin/wokwi-cli.exe) ja ympäristömuuttujan WOKWI_CLI_TOKEN.

.EXAMPLE
  ./scripts/sim.ps1                    # 3 s simulaatio
  ./scripts/sim.ps1 -TimeoutMs 10000
#>
param(
    [int]$TimeoutMs = 3000
)
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Cli = Join-Path $HOME ".wokwi\bin\wokwi-cli.exe"
$Elf = Join-Path $Root "firmware\build\zephyr\zephyr.elf"
$SimDir = Join-Path $Root "firmware\build\sim"

if (-not (Test-Path $Cli)) { Write-Error "Wokwi CLI puuttuu: $Cli" }
if (-not (Test-Path $Elf)) { Write-Error "Firmware puuttuu: $Elf. Käännä ensin (scripts/build.ps1)." }

New-Item -ItemType Directory -Force $SimDir | Out-Null
# Wokwi lukee polut suhteessa wokwi.toml-tiedoston kansioon (build/sim → build/zephyr).
@"
[wokwi]
version = 1
firmware = '../zephyr/zephyr.elf'
elf = '../zephyr/zephyr.elf'
"@ | Set-Content -Encoding utf8 (Join-Path $SimDir "wokwi.toml")

& $Cli $SimDir --diagram-file (Join-Path $Root "diagram.json") --timeout $TimeoutMs --timeout-exit-code 0
exit $LASTEXITCODE
