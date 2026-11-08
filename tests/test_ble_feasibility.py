# SPDX-License-Identifier: GPL-3.0-only
"""Guard the baseline maths and keep this gate in sync with the linker map."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("ble_feasibility", ROOT / "tools" / "ble_feasibility.py")
assert spec is not None and spec.loader is not None
import sys
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


class BudgetTests(unittest.TestCase):
    def test_app_slot_constant_matches_real_package_builder(self):
        source = (ROOT / "tools" / "fm1pkg_make.py").read_text(encoding="utf-8")
        self.assertIn("APP_SLOT = 0x8DFBC", source)
        self.assertEqual(module.APP_SLOT, 0x8DFBC)

    def test_pinned_headroom(self):
        self.assertEqual(module.available(module.Baseline()), {
            "flash": 20324, "ram": 932, "pool": 1976
        })

    def test_upstream_lower_bound_exceeds_budget(self):
        self.assertEqual(
            module.savings_needed(module.Baseline(), module.BLE_FLASH_LOW, module.BLE_RAM, module.BLE_POOL),
            {"flash": 21660, "ram": 9308, "pool": 0}
        )

    def test_upstream_upper_bound_exceeds_budget(self):
        self.assertEqual(
            module.savings_needed(module.Baseline(), module.BLE_FLASH_HIGH, module.BLE_RAM, module.BLE_POOL),
            {"flash": 26780, "ram": 9308, "pool": 0}
        )

    def test_last_build_in_log_is_selected(self):
        log = (
            "ok image 561236 B; RAM .data+.bss 68700 B of 73728; pool 358472 B of 368640\n"
            "ok image 561240 B; RAM .data+.bss 68700 B of 73728; pool 358472 B of 368640\n"
        )
        self.assertEqual(module.read_build_log(log), module.Baseline())

    def test_wrong_layout_is_rejected(self):
        wrong = "image 100 B; RAM .data+.bss 100 B of 99999; pool 100 B of 368640"
        with self.assertRaises(ValueError):
            module.read_build_log(wrong)

    def test_negative_cost_rejected(self):
        with self.assertRaises(ValueError):
            module.savings_needed(module.Baseline(), -1, 1, 1)

    def test_fitting_model_requires_real_headroom(self):
        reduced = module.Baseline(
            image=module.APP_SLOT - 50000,
            ram=module.RAM_LIMIT - module.RAM_RESERVE - 14000,
            pool=module.POOL_LIMIT - module.POOL_RESERVE - 2000
        )
        self.assertEqual(
            module.savings_needed(reduced, module.BLE_FLASH_HIGH, module.BLE_RAM, module.BLE_POOL),
            {"flash": 0, "ram": 0, "pool": 0}
        )


if __name__ == "__main__":
    unittest.main()
