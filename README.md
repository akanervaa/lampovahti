# Lämpövahti: a small Zephyr thermostat

[![tests](https://github.com/akanervaa/lampovahti/actions/workflows/tests.yml/badge.svg)](https://github.com/akanervaa/lampovahti/actions/workflows/tests.yml)

A temperature controller for something like a terrarium or an aquarium. It reads an I2C
temperature sensor, switches a heater with hysteresis, raises an alarm when something goes
wrong, and can be inspected and adjusted from a shell. Built from scratch on **Zephyr RTOS
(v4.5)** for the **Raspberry Pi Pico (RP2040)** and run in the **Wokwi** simulator.

*Lämpövahti* is Finnish for "temperature guard". The code comments and the design document
are in Finnish ([README in Finnish](README.fi.md)). This page summarises them in English.

## What it does

| | Requirement |
|---|---|
| F1 | Reads the temperature once per second (I2C sensor, waits for data-ready with a timeout). |
| F2 | Heater on below the target band, off above it, unchanged inside it (hysteresis). |
| F3 | Alarm (blinking LED) when the temperature stays outside target ± 2 °C for 30 s. |
| F4 | A button acknowledges the alarm (snooze: it comes back if the problem continues). |
| F5 | Shell: `lampo status`, `lampo target 23.5`, `lampo history`. |
| F7, F8 | Sensor fault, or a physically impossible reading or jump → heater off, alarm at once. |
| F9 | Two layers of supervision: a silent sensor is detected in software, and a hardware watchdog resets the chip if the control loop hangs. |

The safe state is always **heater off**: at boot, on any fault, and after a watchdog reset.

## Architecture

```
 MYS-1 ──I2C──► sensor thread ──EV_SAMPLE / EV_SENSOR_FAULT──┐
 button ──IRQ──► debounce (work queue) ──EV_BUTTON───────────┤
 shell ──────────────────────────────────EV_SET_TARGET───────┼──► mailbox (k_msgq) ──► control thread
                                                             │                         │
                                   EV_TICK (1 s timeout) ────┘          decide() ◄─────┤  pure C, no Zephyr
                                                                                       ├──► heater LED, alarm LED (k_timer)
                     shell ◄── snapshot (the only mutex) ◄─────────────────────────────┤
                                                                                       └──► feeds the watchdog
```

- **Functional core, imperative shell.** All decisions live in `core/` as plain C with no
  Zephyr calls: `decide(state, event, now_ms) → actions`. Time is a parameter, so tests can
  feed any time, including the 32-bit millisecond counter wrapping around. The Zephyr code in
  `firmware/` only waits, reads the clock, drives pins and logs.
- **One writer.** Only the control thread changes the state. Everything else posts events to its
  mailbox, so there are no locks around the logic. The shell reads a snapshot behind the
  system's single mutex.
- **Nothing waits where it must not.** Every sender posts with `K_NO_WAIT` (the button runs
  from an interrupt). The ISR only reschedules a 30 ms debounce work item.
- **The watchdog is fed by what it is meant to prove.** Only the end of the control loop feeds
  it. A separate feeder thread would keep feeding while the control loop is stuck.

## Testing

43 unit tests on the PC with [Unity](https://github.com/ThrowTheSwitch/Unity), run in CI on
every push:

- table tests for edge cases (hysteresis band edges, plausibility limits, `<` vs `<=`);
- event-chain tests for behaviour over time (alarm delay, snooze, acknowledgement, a silent
  sensor while someone keeps pressing the button);
- randomised tests with fixed seeds for the shell input parser: 10 000 round trips and
  100 000 garbage strings;
- every test has been seen to fail. The tests were also checked by mutating the code. That
  is how a gap was found: `"99999999999"` was rejected only by luck. `"4294967319"` (2³² + 23)
  was added, because without the overflow guard it would silently become 23.00 °C.

## Known limitations

- **The watchdog reset is not verified.** Wokwi runs the RP2040 watchdog counter down but never
  resets the chip (checked with a spike, `spikes/watchdog`). It needs a real Pico, or Renode.
- **The simulated sensor plays a fixed sequence** (22.50 … 22.93 °C), so there is no thermal
  feedback from the heater. A first-order plant model (C·dT/dt = P·u − (T − T_out)/R) in the
  simulated chip would close the loop.
- **Simulator timing differs from real time.** Some timeouts are overridden in `prj.conf`
  for Wokwi. The real-hardware defaults are in `firmware/Kconfig`.
- **The history keeps the last 32 events**, so a burst of samples can push an old alarm out.
  The log keeps the full record.

## Build and run

Needs a Zephyr v4.5 workspace, the Arm GNU toolchain, CMake, Ninja and the Wokwi VS Code
extension (or Wokwi CLI). The scripts are PowerShell.

| | |
|---|---|
| Unit tests (PC) | `./scripts/test.ps1` |
| Build firmware | `./scripts/build.ps1` |
| Simulate | VS Code: *Wokwi: Start Simulator*, or `./scripts/sim.ps1` |

In the simulator, type `lampo status` in the serial monitor and press the blue button (or
key **K**) to acknowledge an alarm.

## Layout

```
core/        decision logic, plain C (hysteresis, plausibility, alarm, history, parser)
tests/       Unity tests for core/
firmware/    Zephyr application: threads, drivers, devicetree overlay, Kconfig
chips/       simulated MYS-1 I2C sensor for Wokwi (from the embedded course this was built in)
spikes/      throwaway experiments that answered design questions
DESIGN.md    requirements, architecture and every decision with rejected alternatives (Finnish)
```
