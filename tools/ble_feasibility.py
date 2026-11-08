#!/usr/bin/env python3
"""Read-only BLE-MIDI integration size gate for Caravel Plus.

This estimates resource pressure; it does not compile or enable a radio.
The default baseline comes from the exact plus2 source build on 2026-11-07,
CI run 42377151515, with 4 KiB RAM / 8 KiB pool reserves preserved.

Upstream fm1-base's own BLE stack figures are *template measurements*:
41-46 KiB flash, ~10 KiB RAM, ~1 KiB pool. They are not a prediction that
a port into Caravel will have identical costs.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re

APP_SLOT = 0x8DFBC
RAM_LIMIT = 72 * 1024
POOL_LIMIT = 0x5A000
RAM_RESERVE = 4096
POOL_RESERVE = 8192

# Caravel Plus v0.11.0-plus2; GitHub workflow run 42377151515, job wasm.
BASE_IMAGE = 561240
BASE_RAM = 68700
BASE_POOL = 358472

# Baseline upstream template costs: not a completed Caravel integration.
BLE_FLASH_LOW = 41 * 1024
BLE_FLASH_HIGH = 46 * 1024
BLE_RAM = 10 * 1024
BLE_POOL = 1 * 1024

# The exact phrase printed by tools/build.py's resource checker.
BUILD_LOG = re.compile(
    r"image\s+(\d+)\s+B;\s+RAM\s+\.data\+\.bss\s+(\d+)\s+B"
    r"\s+of\s+(\d+);\s+pool\s+(\d+)\s+B\s+of\s+(\d+)"
)


@dataclass(frozen=True)
class Baseline:
    image: int = BASE_IMAGE
    ram: int = BASE_RAM
    pool: int = BASE_POOL

    def __post_init__(self) -> None:
        if min(self.image, self.ram, self.pool) < 0:
            raise ValueError("Memory readings must be nonnegative")


def read_build_log(content: str) -> Baseline:
    """Prefer the final successful size check in a CI log, e.g. plus2."""
    matches = list(BUILD_LOG.finditer(content))
    if not matches:
        raise ValueError("No Caravel image/RAM/pool size line in build log")
    image, ram, ram_total, pool, pool_total = map(int, matches[-1].groups())
    if ram_total != RAM_LIMIT or pool_total != POOL_LIMIT:
        raise ValueError("Linker memory map changed; review limits before porting")
    return Baseline(image, ram, pool)


def available(b: Baseline) -> dict[str, int]:
    """Free bytes while keeping existing non-negotiable safety reserves."""
    return {
        "flash": APP_SLOT - b.image,
        "ram": RAM_LIMIT - RAM_RESERVE - b.ram,
        "pool": POOL_LIMIT - POOL_RESERVE - b.pool,
    }


def savings_needed(b: Baseline, flash_add: int, ram_add: int, pool_add: int) -> dict[str, int]:
    """Minimum bytes that must be saved before adding the estimated stack."""
    if min(flash_add, ram_add, pool_add) < 0:
        raise ValueError("Stack cost cannot be negative")
    room = available(b)
    return {
        "flash": max(0, flash_add - room["flash"]),
        "ram": max(0, ram_add - room["ram"]),
        "pool": max(0, pool_add - room["pool"]),
    }


def report(b: Baseline) -> str:
    room = available(b)
    low = savings_needed(b, BLE_FLASH_LOW, BLE_RAM, BLE_POOL)
    high = savings_needed(b, BLE_FLASH_HIGH, BLE_RAM, BLE_POOL)
    return "\n".join([
        "Caravel Plus / BLE-MIDI feasibility — ESTIMATE, NOT AN INSTALLABLE BUILD",
        f"baseline: flash {b.image}/{APP_SLOT}; RAM {b.ram}/{RAM_LIMIT}; pool {b.pool}/{POOL_LIMIT}",
        f"headroom after required reserves: flash {room['flash']} B, "
        f"RAM {room['ram']} B, pool {room['pool']} B",
        "fm1-base own BLE stack: flash +41..46 KiB, RAM ~+10 KiB, pool ~+1 KiB (template only)",
        f"minimum savings needed (lower-cost estimate): "
        f"flash {low['flash']} B; RAM {low['ram']} B; pool {low['pool']} B",
        f"minimum savings needed (upper-cost estimate): "
        f"flash {high['flash']} B; RAM {high['ram']} B; pool {high['pool']} B",
        "BLOCKED: no Caravel radio integration has been built, linked or tested.",
        "Never flash a feasibility report or treat upstream template sizes as a guarantee.",
    ])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, help="Caravel build log (last size check wins)")
    parser.add_argument(
        "--require-fit", action="store_true",
        help="return nonzero while the upper-bound estimate exceeds the limits"
    )
    args = parser.parse_args()
    try:
        b = read_build_log(args.log.read_text(encoding="utf-8", errors="replace")) if args.log else Baseline()
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(report(b))
    high = savings_needed(b, BLE_FLASH_HIGH, BLE_RAM, BLE_POOL)
    return 2 if args.require_fit and any(high.values()) else 0


if __name__ == "__main__":
    raise SystemExit(main())
