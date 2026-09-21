#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import kscreen_topology as module


SAMPLE = """\
Output: 1 DP-3 uuid
\tGeometry: 1920,0 2560x1080
Output: 2 DP-2 uuid
\tGeometry: -1280,-200 1280x1024
Output: 3 HDMI-A-1 uuid
\tGeometry: 0,1080 1920x1080
"""


class KScreenTopologyTest(unittest.TestCase):
    def test_parses_dynamic_names_and_negative_coordinates(self):
        outputs = module.parse_outputs(SAMPLE)
        self.assertEqual([output.name for output in outputs], ["DP-3", "DP-2", "HDMI-A-1"])
        self.assertEqual((outputs[1].x, outputs[1].y), (-1280, -200))

    def test_calculates_virtual_bounds_without_reference_monitor_constants(self):
        outputs = module.parse_outputs(SAMPLE)
        self.assertEqual(module.virtual_bounds(outputs), (-1280, -200, 5760, 2360))

    def test_rejects_missing_geometry(self):
        with self.assertRaises(ValueError):
            module.virtual_bounds([])


if __name__ == "__main__":
    unittest.main()
