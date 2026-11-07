# Caravel Plus — development plan

Caravel Plus is an experimental, community-derived usability extension of [Caravel](https://github.com/bonszaimusic/caravel) for the M-VAVE FM-1. It begins from the Caravel v0.11.0 codebase.

**Status:** design / baseline review only. No firmware changes or hardware verification have been performed by this branch.

## Principles

1. Preserve the v0.11.0 synth engines, sequencing, project storage, and performance controls.
2. Keep `main` as the unmodified upstream baseline. Implement changes on feature branches and review them via pull request.
3. Prove UI behaviour on host and browser emulator before any on-device flash.
4. Do not claim MIDI host compatibility, Bluetooth support, or physical-device performance without testing.
5. Retain the original project authors' notices, GPL-3.0-only licence, and full corresponding-source obligations.

## Phase 1: Discoverable four-track mixer

**Problem:** Navigating HOME does not expose an obvious MIXER screen, and users struggle to adjust tracks together.

**Design target:**
- One discoverable MIXER screen showing four tracks simultaneously.
- Selected-track indication and unambiguous physical controls.
- Per-track level and mute as first delivery; evaluate pan/solo only after confirming existing audio routing and headroom.
- MIDI, sequencing, audio and preset compatibility retained.
- Transport keeps running while mixing.
- No implicit project overwrite; changes participate in the existing save/load model.

**Before implementation:**
- Locate existing track level/mute state and UI navigation paths in `firmware/src/`.
- Determine whether pan or solo are already supported in the signal chain.
- Identify drawing helpers and encoder/key binding collisions; document navigation affordance.
- Establish UI host tests and browser emulator screenshots for baseline and modified layouts.
- Verify native target firmware image and RAM budgets against `BUILDING.md`.
- Verify saved project compatibility and reboot behaviour.

**Acceptance criteria:**
- A new user can find MIXER without consulting hidden combinations.
- Four track labels and levels are readable on the 240×240 screen.
- Changing level/mute does not affect another track or lose playing sequences.
- Saved project reloads expected mixer settings.
- Host checks and web emulator pass; no firmware package released before hardware acceptance.

## Phase 2: UI polish and sequencing

Make track selection, step entry, illuminated key feedback, and save/overwrite confirmation clearer, with regression tests.

## Phase 3: External integration

Investigate MIDI clock and channel routing for 1010music Bento via a supported wired connection. Treat Bluetooth and direct USB-host compatibility as unverified until demonstrated on both devices.

## Phase 4: Experimental sound design

Explore generative sequencing and optional engines **only after** base stability and performance have been measured.

## Build and recovery discipline

- Preserve user's current Caravel v0.11.0 package and export WORK/A-D projects and user banks before flashing.
- Clone with `git clone --recurse-submodules https://github.com/jimm7/caravel.git`.
- Use `BUILDING.md` and pinned SDK/toolchain prerequisites; compile and test before distributing any `.fwsc`.
- Evaluate firmware flashing and recovery steps using `docs/faq.md`; beta firmware is at the owner's risk.
- Keep upstream notices and licences; distribute corresponding sources with any binary.

This file is a roadmap, **not** a claim that implementation or hardware testing is complete.

## First implementation: discoverable existing mixer (2026-11-07)

The v0.11.0 source already implements a MIXER page with K1 LEVEL, K2 PAN, K3 REV send and K4 MUTE for the track selected using ALGORITHM. The first firmware edit makes that **existing page** accessible without adding a second mixing subsystem:

- From performance HOME, tap HOME to open MIXER.
- From MIXER, tap HOME to show CLOCK.
- From CLOCK (or any other non-MIXER page), tap HOME to return to performance HOME.
- ALGORITHM changes selected track. The existing K1-K4 mixer functions are unchanged.
- SAVE > PROJECT > A-D remains the intended way to persist the performance.

**Testing status:** code review only. Host compilation, emulator checks, button-event regression tests, and hardware acceptance are not yet completed. Do not flash this development commit.

**Follow-up:** inspect the double-tap interception in `ui_input.c` and how HOME track-navigation preferences interact with this behaviour; verify tap vs held-menu behaviour before release. Design a four-track simultaneous mixing view only after the navigation test is sound.
