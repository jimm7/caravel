# Caravel Plus Bluetooth MIDI — stage 0: capacity and interface review

**Status: NOT IMPLEMENTED. No firmware package produced. Not approved to flash.**

This is an engineering gate, not a Bluetooth-on switch. Development starts from `feature/caravel-plus-foundation`, where Caravel Plus 0.11.0-plus2 added the HOME/MIXER/LEVELS UI and support for the UCUN UC25HQ80 flash ID. **Do not replace the user's working firmware or overwrite projects while this work is under development.**

## Why this is a substantial port

Caravel includes a standalone HAL (`firmware/hal/`), a single C translation unit (`firmware/src/felucca.c`), a TIMER5 keyboard/USB/MIDI ISR, and its own app linker layout and flash store. The `fm1-base` BLE stack was built for **another platform API**, `plat.h`, with the `felucca-bsp` low-level radio drivers. It cannot simply be enabled using a `#define` in Caravel.

The upstream project's own Bluetooth implementation is currently the most promising option:

- Source: https://github.com/charlesvestal/fm1-base
- Stack docs: https://github.com/charlesvestal/fm1-base/blob/main/docs/bt/README.md
- The upstream implementation (Henri David's Optimist stack with Charles Vestal's glue) is MPL-2.0; preserve its file headers, credits and source terms. Caravel remains GPL-3.0-only, and any combined redistribution must be reviewed for its applicable obligations. Do not copy JieLi's closed-source libraries into this repository.
- Reference settings: `ble = true`, `ble_stack = "own"`; BLE-MIDI **peripheral** to a phone, no A2DP, no central-role scanning, no Wi-Fi, no pairing until essential BLE-MIDI works.
- Upstream's own radio uses low-level hardware interrupts for its stack and rings for MIDI. Radio driver, interrupt vectors, RF calibration, timers, startup and stores are not present in Caravel's HAL and require an explicit port, not just app changes.

## Hard memory numbers

Measured on the actual `jimm7/caravel` Plus source: GitHub ACTION banks Android emulator run [42377151515](https://github.com/jimm7/caravel/actions/runs/42377151515), final v0.11.0-plus2 target build:

| Region | Used | Limit | Spare | Safe reserve | Available for new feature |
| --- | ---: | ---: | ---: | ---: | ---: |
| App image | 561,240 B | 581,564 B | 20,324 B | 0 | 20,324 B |
| Main RAM | 68,700 B | 73,728 B | 5,028 B | 4,096 B | 932 B |
| Pool | 358,472 B | 368,640 B | 10,168 B | 8,192 B | 1,976 B |

From the `fm1-base` README **on its small template**: own BLE stack adds about **41–46 KiB image**, **10 KiB main RAM**, **1 KiB pool**. At the upper end, **at least 26,780 B of flash and 9,308 B of RAM must first be saved** (assuming no other cost, so actual port may need more). The current layout is already over the reserve-adjusted budget for that stack. `python3 tools/ble_feasibility.py` prints the calculation, `--log <build-log>` uses a newer baseline, and `--require-fit` fails if the model is too large. This tool is read-only.

## Port boundaries and acceptance order

1. **Protect existing baseline.** Keep full source, installed package, loader, recovered original firmware, known-good device identity `FM-1_999` and two independently verified backups of WORK/A–D/projects/presets. We are not changing the installed unit, storage geometry, product identity, or bootloader as part of stage 0.
2. **Reclaim flash and main RAM without deleting user-facing features.** Measure exact savings with the target compiler, app image size, RAM and pool. Investigate constants, redundant buffers, shared scratch and optimisations, with audio/USB/persistence regression tests; never reclassify flash-overwrite zones or silently lower 4/8 KiB reserves.
3. **Port hardware safely.** Under an opt-in build flag (default off), map `fm1-base`'s radio interrupts, timer, clock, RF calibration, controller initialization and linker symbols to the FM-1's existing Caravel HAL without conflicting with TIMER5, audio IRQ, worker core or flash-off routines. Keep a separate **USB-only image** for rollback.
4. **Bridge MIDI through the right contexts.** Caravel's `midi_enqueue` (`firmware/src/usb.c`) writes a 64-packet ring whose single producer is TIMER5 (USB/TRS), and `seq.c` consumes it during the audio ISR. BLE IRQs **must not directly call `midi_enqueue`**: first use the BLE stack's own single-producer receive ring, then drain it from TIMER5 as the one existing MIDI producer. Add source ID **3 BLE**; keep USB=1, TRS=2. The current clock-source setting only recognises USB/TRS, so BLE clock routing and clock/source UI require separate changes and tests. MIDI transmit must similarly support BLE without regressing USB.
5. **Make startup recoverable.** Keep BLE disabled after failed boots; report radio calibration failures without bricking or entering a restart loop. Use a sound, non-overlapping store for any Bluetooth identity/bond without touching the saved Caravel projects or original firmware flash/calibration sectors.
6. **Automated validation.** Compile both USB-only and BLE variants, reproduce both app and loader, check both linker budgets and required reserves, run existing Caravel Complete Validation, and add BLE ring, note-off, disconnect panic, clock loss, MIDI-CC and MIDI timing tests. Investigate crashes, interrupted USB, buffer saturation, and audio dropouts.
7. **Hardware acceptance gate.** Only after verified backups and a recovery plan, do a controlled, **manual, opt-in** FM-1 trial. Confirm boot, four-track LEVELS/MIXER, sound, sequencer, project save/reboot/load (including UC25HQ80), USB, TRS and BLE advertising, Android MIDI service UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`, Android MIDI characteristic, note on/off, clock, range and latency. Never infer BLE success just because Android created a virtual MIDI port.

## Android companion

`jimm7/FM1-Companion` provides a phone-side BLE scanner and diagnostic. v0.1.1 fixed Android BLE scanning on the Samsung S24 Ultra. Its earlier "connected" result with `BT-TH-161C5C58` is **not evidence** that Caravel advertised MIDI; that device advertised `FFD0`, not the standard BLE-MIDI service. The Android application now has a branch with explicit GATT service verification.

## Stage 1: MIDI receive bridge prototype (source only)

`firmware/src/ble_midi_bridge.c` is included only with `CARAVEL_BLE_MIDI_BRIDGE=1` (default is 0). It establishes the future BLE stack's receive-contract: one radio ISR producer submits **already decoded** 4-byte MIDI 1.0 events to its own bounded ring; TIMER5 drains the ring and is still the only producer of `midi_enqueue`. The events are tagged source **3** (USB=1, TRS=2) and existing `seq.c` will route note/CC messages. SysEx, vendor data and malformed status bytes are rejected; saturation requests the established note-release/panic recovery. BLE external clock selection, outbound MIDI and all RF functionality **remain absent**.

The adapter is tested without the radio through `tests/ble_midi_bridge_host.c`, compiled in CI by the host C compiler. **Target** `tools/build.py` refuses an enabled bridge build because installing it without the hardware stack would falsely imply BLE works. A source file under `firmware/src/` is not evidence of a BLE-capable `.fwsc`.

## Release rule

**Nothing in this stage enables Bluetooth or changes a synth firmware image.** Keep this PR in draft and never merge it as "BLE support" or issue `.fwsc`/flash instructions until compilation, resource checks, storage compatibility and on-device safety are demonstrated. A green *feasibility* CI is only evidence the calculations and documentation passed, not proof that a BLE firmware exists.
