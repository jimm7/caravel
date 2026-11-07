# Caravel Plus v0.11.0-plus1 — beta candidate checklist

**STATUS: UNRELEASED, UNAPPROVED FOR INSTALLATION. DO NOT FLASH THIS ARTIFACT.**

This development package is for code review, package-integrity checks and controlled preflight planning. Passing automated tests does not prove that it will boot, preserve storage or recover on a physical M-VAVE FM-1. No physical-device acceptance has been performed.

## What it contains

- Caravel Plus changes to the performance HOME navigation and new four-track LEVELS view (K1–K4 control track 1–4 volume)
- Existing selected-track MIXER, ACTION-bank interactions, and other original synthesis/sequencing features
- An explicitly versioned, rebuilt firmware with the on-device version string `v0.11.0-plus1` and versioned build identity `FM-1_999` (this differs from the earlier development artifact)
- `BUILD-COMMIT.txt` identifies the *actual GitHub runner checkout*; on pull requests this may be a synthetic merge commit, rather than the branch tip in `FEATURE-HEAD-COMMIT.txt`
- `SHA256SUMS` allows corruption checks but is not a publisher signature or proof that flashing is safe

## Software validation prerequisites

1. The **same feature-branch revision** must pass Source and release metadata, ACTION banks Android emulator and Caravel Complete Validation.
2. The candidate workflow must pass `tools/build.py --binary-version 0.11.0-plus1` and `release_artifacts.verify_binaries`, including pinned SDK hash checks, app identity/version, loader and .fwsc package consistency.
3. Confirm package name and SHA-256 in the build artifact, and review the GitHub job logs for RAM/flash-budget or compiler failures.
4. Confirm no unexpected page-index or project-format changes. Retain test evidence.

## Physical-device prerequisites — not yet satisfied

1. Make an **offline copy** of your current Caravel v0.11.0 firmware package and confirm its SHA-256. Keep the official M-VAVE firmware package separately.
2. **Back up all WORK and A–D projects, presets and settings**, using a matching backup tool for the installed firmware. Independently inspect the backup for all expected slots and store two copies off the device. A downloaded firmware ZIP is **not** a backup of the FM-1.
3. Before upgrading, read `docs/installing.md` and `docs/faq.md` in full. Verify the real FM-1's device identity, update/recovery-mode entry, compatible data cable, stable power, and USB connection without changing firmware.
4. Know the recovery differences: ordinary Caravel USB-MIDI update mode, chip UBOOT mode, and independent tools like M-Flasher. Recovery is **not guaranteed**; an interrupted or bad image can leave the instrument unusable.
5. Have a verified restore route for **your specific hardware and source firmware**, plus an independent original firmware file. Do not assume downgrading preserves data or that official firmware understands Caravel project formats.
6. Complete on-device acceptance plan: boot, screen, four-track LEVELS and MIXER navigation; K1–K4 independence; fast/slow HOME taps, ACTION, MENU; transport, sound and four-track output; MIDI clock; project save/load; USB recognition; extended operation; safe reboot. Test the restored data after any recovery procedure.
7. Do **not** use `--force`, `--yes`, or unknown firmware files in a first trial.

## Release decision

No automated green check constitutes approval to install. An installation candidate requires a deliberate decision after backup integrity, recovery route and hardware-specific risks have been reviewed. Keep PR #1 in draft; do not publish a GitHub Release or merge into `main` until accepted.

Sources: [Project PR](https://github.com/jimm7/caravel/pull/1), [installation instructions](../docs/installing.md), [recovery FAQ](../docs/faq.md), [build documentation](../BUILDING.md).
