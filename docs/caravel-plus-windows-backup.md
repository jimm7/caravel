# Caravel Plus: Windows FM-1 backup preflight (read-only first)

**This procedure does not install firmware. Do not flash the beta candidate yet.** The steps below assume a physical FM-1 already running a compatible Caravel performance firmware. If your FM-1 is still on stock M-VAVE firmware, or an earlier/incompatible firmware, do not use the Caravel backup command until its protocol is confirmed.

## What is backed up

The `tools/caravel_backup.py save` command reads and saves 13 object slots: WORK, SETTINGS (including MIDI Learn), A–D performance projects, two preset partitions, legacy FM6, FM6 and three user-sample slots. It does not claim to be a **whole-flash backup**, and is not a substitute for saving the original firmware or knowing the recovery procedure.

## Windows preparation

1. Put a copy of the repository on your Windows PC, for example using **Code > Download ZIP** from the Caravel Plus branch (a clone with submodules is better for actual builds, but the backup tool can run without them). Open PowerShell in the extracted repo folder.
2. Connect FM-1 by a known-good USB data cable; ensure stable power, stop transport and close any competing MIDI editor.
3. Check Python and install MIDI dependencies (Windows):

   ```powershell
   py -3 --version
   py -3 -m pip install mido python-rtmidi
   ```

4. Identify MIDI ports without writing to the instrument:

   ```powershell
   py -3 tools/caravel_backup.py ports
   ```

5. On a *compatible Caravel performance firmware only*, back up to a local directory:

   ```powershell
   New-Item -ItemType Directory -Force -Path "$HOME\Documents\FM1-Backups" | Out-Null
   $backup = Join-Path "$HOME\Documents\FM1-Backups" ("fm1-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".zip")
   py -3 tools/caravel_backup.py save "$backup"
   ```

   If auto-detection is ambiguous, run `ports` and specify the two **exact** names with `--input "..." --output "..."` before `save`. Do not enter a firmware-installation command as part of this backup.

6. Independently check the archive **offline** (no MIDI port is opened, and no device is written to):

   ```powershell
   py -3 tools/caravel_backup.py verify "$backup"
   Get-FileHash -Algorithm SHA256 "$backup" | Format-List
   ```

   Expect an explicit `OFFLINE CHECK PASSED` and all 13 named objects. Record its file size, the full SHA-256 and the list of present project slots, and keep **two copies on separate storage**. A successful file integrity check cannot guarantee that every desired slot had the intended musical content—inspect your projects on the device separately.

7. Retain the *currently installed* official or Caravel firmware package and its checksum, plus M-VAVE's unmodified official `FM-1.fwsc` separately. Consult `docs/installing.md` and `docs/faq.md` for failure and recovery risks. If you cannot demonstrate a reasonable return route for your actual device, **do not flash**.

## Important safety limits

- Never run `restore`, `import`, the installer, `--force` or `--yes` as part of this preflight.
- The ZIP verifier performs manifest, declared size, CRC and per-object content validation. This is an integrity check, not a proof of successful recovery or hardware compatibility.
- Unknown or unsupported firmware/device identities are a STOP condition.
- A backup may contain private musical work and device data; do not post it publicly as a GitHub issue or workflow artifact.
- For your first hardware trial, use a supervised acceptance plan and ensure the FM-1 can recover *before* an update is attempted.

Repository references: `tools/caravel_backup.py`, `docs/caravel-plus-beta-checklist.md`, `docs/installing.md` and `docs/faq.md`.
