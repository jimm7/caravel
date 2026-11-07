/* SPDX-License-Identifier: GPL-3.0-only
 * Host regression: FM-1 flash safety gate accepts only documented
 * 1 MiB Puya and UCUN parts. No flash writes are performed.
 */
#include <assert.h>
#include <stdint.h>
#include "../firmware/hal/fm1_flash_ids.h"

int main(void)
{
    assert(fm1_flash_jedec_supported(0x856014u)); /* Puya P25Q80H */
    assert(fm1_flash_jedec_supported(0xB36014u)); /* UCUN UC25HQ80 */
    assert(!fm1_flash_jedec_supported(0u));
    assert(!fm1_flash_jedec_supported(0xFFFFFFu));
    assert(!fm1_flash_jedec_supported(0xEB6014u)); /* other model */
    assert(!fm1_flash_jedec_supported(0xEF4014u)); /* unknown chip */
    assert(!fm1_flash_jedec_supported(0xB36015u)); /* different density */
    return 0;
}
