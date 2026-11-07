/* SPDX-License-Identifier: GPL-3.0-only
 * Known FM-1 1 MiB SPI-NOR JEDEC identities.
 *
 * The original FM-1 used Puya P25Q80H (0x856014).
 * Some late-2026 FM-1 units use UCUN UC25HQ80 (0xB36014),
 * as observed by M-Flasher. Do NOT broaden this to arbitrary 1 MiB
 * chips: this is a firmware storage / OTA safety gate.
 *
 * Accepting an ID does not itself validate write/erase timing on every
 * board revision. Physical flash save, readback, reboot, and rollback
 * acceptance is still required before releasing support.
 */
#pragma once
#include <stdint.h>

#define FM1_FLASH_JEDEC_PUYA_P25Q80H 0x856014u
#define FM1_FLASH_JEDEC_UCUN_UC25HQ80 0xB36014u

static inline int fm1_flash_jedec_supported(uint32_t id)
{
    return id == FM1_FLASH_JEDEC_PUYA_P25Q80H ||
           id == FM1_FLASH_JEDEC_UCUN_UC25HQ80;
}
