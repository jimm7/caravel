# Caravel Plus BLE-MIDI — memory audit and optimisation candidates

**Engineering research only. No new firmware binary and no Bluetooth RF enablement.**

## Reproducible baseline

`jimm7/caravel` branch `feature/caravel-plus-foundation`, CI run [42377151515](https://github.com/jimm7/caravel/actions/runs/42377151515), final compiled `v0.11.0-plus2`:

- App image: **561,240 / 581,564 bytes**, leaving 20,324 bytes.
- Main `.data + .bss`: **68,700 / 73,728 bytes**, leaving 5,028 bytes, or **932 bytes after Caravel's existing 4 KiB safety reserve**.
- `.pool`: **358,472 / 368,640 bytes**, leaving 10,168 bytes, or **1,976 bytes after the existing 8 KiB safety reserve**.

The `fm1-base` reference on *its own small firmware template* adds about **41–46 KiB flash, 10 KiB main RAM and 1 KiB pool** for the own BLE-MIDI stack. Thus, at the upper reference cost, the initial Caravel budget is short **26,780 bytes of flash and 9,308 bytes of main RAM**. These are **planning estimates**, not a linker result for a Caravel port.

## Large allocation candidates found in source

These are *currently allocated sizes*, **not** space reclaimed.

| Source | Allocation | Bytes | What it does / constraints |
| --- | --- | ---: | --- |
| `firmware/src/gfx.c` | `cv_px[240 * 124]` (RGB565) | **59,520 pool** | Off-screen graphics canvas. Used by all the canvas drawing primitives and passed to `lcd_blit`. The LCD may continue reading the buffer asynchronously; `cv_begin` calls `lcd_sync`. Do **not** simply reuse this RAM during DMA or shorten it without a tiled-rendering design and visual parity tests. |
| `firmware/src/fx.c` | `dly_buf[65536]` (int16) | **131,072 pool** | Tempo delay buffer, also reused by STASIS. Reducing it may break BPM/division and STASIS behaviour, so not the first optimisation candidate. |
| `firmware/src/slicer.c` | `sl_buf[4][4096]` (int16) | **32,768 pool** | SLICER delay/recording state for all four tracks. Do not collapse tracks or reduce its duration without explicit product acceptance. |
| `firmware/src/storage.c` | `st_buf[4096 - 256]` (byte) | **3,840 pool** | Temporary flash-store buffer. Potential lifetime overlap with graphics must be proven (save/recovery + asynchronous LCD DMA) before sharing it. |

A large pool allocation is **not automatically interchangeable with main RAM**: Caravel's linker divides the shared SRAM at an explicit boundary (`firmware/app.ld`), which the build checker validates, and its stack/guard/NOINIT areas cannot be consumed. Making graphics more memory efficient could enable a **separately measured** RAM/POOL repartition, but saves no `.bss` bytes by itself.

## Best preservation-first experiment

1. Keep a baseline visual-test corpus of screen captures and host image checks. In particular: mixer, four-track LEVELS, waveform/graph, editor, keys, palettes, scrolling and the 240-by-124 canvas maximum.
2. Prototype a **tiled/streamed graphics renderer in a non-production flag**, preserving 16-bit RGB565 output and correct `lcd_sync` semantics. Do not cut canvas dimensions or disable views. Re-rendering tiles has CPU/latency implications; measure UI FPS and audio dropouts.
3. If visual and temporal parity passes, measure actual `.pool` savings on the target compiler, then consider changing RAM/POOL region boundaries **while preserving** 4 KiB main-RAM and 8 KiB pool reserves and existing 8 KiB debug window, stacks and boot/NOINIT layout.
4. Rebuild the **unchanged** firmware with the same toolchain and flags for a control comparison. Use `tools/ble_feasibility.py --log <target-build-log>` on each result; model savings only after a real link.
5. Investigate flash-size optimisation separately. Reclaiming pool RAM does **not** recover flash space; target compiler/linker map and image layout are required. No cuts to synthesizer engines or presets as a default.
6. Only after adequate verified RAM, pool and flash headroom: attempt the low-level RF port in a separately labelled, non-installable build, first advertising, then MIDI notes, and finally clock, TX, pairing and recoverable persistent settings.

## Relevant transport and safety details

`firmware/src/usb.c`'s `midi_enqueue` publishes to a 64-packet ring, with MIDI input processed by the audio side in `firmware/src/seq.c`. The experimental BLE receive bridge keeps the radio ISR off this shared queue and drains from TIMER5. On BLE link loss or dropped packets it requests **the existing global MIDI overflow panic**, which may also release USB/TRS-held notes. Source-specific panic/recovery requires additional ownership tracking and tests before a real radio port can be accepted.

The bridge host test can run without an FM-1 or JieLi compiler:

```sh
cc -std=c11 -Wall -Wextra -Werror tests/ble_midi_bridge_host.c -o /tmp/ble-midi-bridge-test
/tmp/ble-midi-bridge-test
python3 -m unittest discover -s tests -p test_ble_feasibility.py -v
python3 tools/ble_feasibility.py --require-fit  # expected nonzero until memory is reclaimed
```

## Do not flash

The `feature/caravel-plus-ble-midi-port` branch contains no Bluetooth RF driver integration and **must not** be installed as if it were BLE-capable. Do not publish or install an `.fwsc` from the experimental branch. The existing on-device firmware, update loader and music are not changed by this work.
