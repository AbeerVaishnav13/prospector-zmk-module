"""Host tests for operator palette; no Zephyr SDK required.

Run: python3 -m unittest discover -s tests -v
"""
import ctypes
from pathlib import Path
import subprocess
import tempfile
import unittest

LAYOUT = Path(__file__).resolve().parents[1] / "boards/shields/prospector_adapter/src/layouts/operator"


class OperatorLayerColorsTest(unittest.TestCase):
    def test_compiled_layer_color_mapping(self):
        with tempfile.TemporaryDirectory() as tmp:
            library = Path(tmp) / "theme.so"
            subprocess.run([
                "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
                str(LAYOUT / "theme_cycle.c"), "-o", str(library),
            ], check=True)
            color = ctypes.CDLL(str(library)).operator_layer_text_color
            color.argtypes = [ctypes.c_uint8]
            color.restype = ctypes.c_uint32
            expected = [0xFFFFFF, 0x00FF00, 0xFF4646, 0xFF7F00, 0x6262FF,
                        0x6262FF, 0xFF34FF, 0xFF34FF, 0x00FFFF, 0x00FFFF]
            for index, rgb in enumerate(expected):
                with self.subTest(layer=index):
                    self.assertEqual(color(index), rgb)
            # Rapid home/nav/home lookup has no mutable shared palette state.
            for index in [0, 4, 0] * 1000:
                self.assertEqual(color(index), expected[index])
            self.assertEqual(color(255), 0xFFFF00)

    def test_white_palette_and_fixed_status_reds(self):
        macros = {}
        for line in (LAYOUT / "display_colors.h").read_text().splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[0] == "#define":
                macros[parts[1]] = parts[2]
        self.assertEqual([macros[f"OPERATOR_COLOR_{tier}"] for tier in
                          ["BG", "DARK", "MED", "BRIGHT"]],
                         ["0x0D0D0D", "0x3B3B3B", "0x8F8F8F", "0xFFFFFF"])
        for name, value in {
            "DISPLAY_COLOR_USB_ACTIVE_BG": "0xCC3333",
            "DISPLAY_COLOR_USB_INACTIVE_BG": "0x6B2020",
            "DISPLAY_COLOR_SLOT_UNPAIRED": "0x6B2020",
            "DISPLAY_COLOR_SLOT_DISCONNECTED_BG": "0xCC3333",
        }.items():
            self.assertEqual(macros[name], value)
        self.assertEqual(macros["DISPLAY_COLOR_BLE_ACTIVE_BG"], "OPERATOR_COLOR_MED")
        self.assertEqual(macros["DISPLAY_COLOR_SLOT_CONNECTED_BG"], "OPERATOR_COLOR_MED")

    def test_layer_callback_only_updates_layer_label(self):
        source = (LAYOUT / "wpm_meter.c").read_text()
        callback = source.split("static void layer_update_cb(", 1)[1].split(
            "static struct layer_state layer_get_state", 1)[0]
        self.assertIn("operator_layer_text_color(state.index)", callback)
        self.assertIn("lv_obj_invalidate(widget->layer_label)", callback)
        self.assertNotIn("lv_obj_invalidate(widget->obj)", callback)
        for source_path in LAYOUT.glob("*.c"):
            text = source_path.read_text()
            self.assertNotIn("theme_cycle_register_refresh", text)
            self.assertNotIn("theme_refresh_retry", text)
            self.assertNotIn("theme_dyn_", text)


if __name__ == "__main__":
    unittest.main()
