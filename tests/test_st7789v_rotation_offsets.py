"""Execute real driver orientation/address-window functions with transport stub."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    start = source.index('static ', source.index(name) - 20)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class DriverRotationOffsets(unittest.TestCase):
    def run_driver(self, body):
        source = (ROOT / 'drivers/display/display_st7789v.c').read_text()
        structs = '\n'.join(re.findall(r'struct st7789v_(?:config|data) \{.*?\n\};', source, re.S))
        # Support old driver (offsets in data only) and fixed driver (immutable config).
        config_offsets = '.x_offset = 0, .y_offset = 20,' if 'uint16_t x_offset;' in structs.split('struct st7789v_data')[0] else ''
        code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
struct device { const void *config; void *data; };
struct mipi_dbi_config { int unused; };
enum display_orientation { DISPLAY_ORIENTATION_NORMAL, DISPLAY_ORIENTATION_ROTATED_90,
 DISPLAY_ORIENTATION_ROTATED_180, DISPLAY_ORIENTATION_ROTATED_270 };
#define LOG_ERR(...) ((void)0)
#define LOG_INF(...) ((void)0)
#define sys_cpu_to_be16(v) (v)
#include "display_st7789v.h"
static int transport_result;
static uint16_t columns[2], rows[2];
static int st7789v_transmit(const struct device *dev, uint8_t cmd, uint8_t *buf, size_t len) {
 (void)dev; (void)len;
 if (cmd == ST7789V_CMD_CASET) { columns[0] = ((uint16_t *)buf)[0]; columns[1] = ((uint16_t *)buf)[1]; }
 if (cmd == ST7789V_CMD_RASET) { rows[0] = ((uint16_t *)buf)[0]; rows[1] = ((uint16_t *)buf)[1]; }
 return transport_result;
}
'''
        code += structs + '\n' + '\n'.join(function(source, name) for name in (
            'st7789v_set_lcd_margins', 'st7789v_set_mem_area', 'st7789v_set_orientation'))
        code += '\nint main(void) {\nconst struct st7789v_config config = {' + config_offsets + '.width = 240, .height = 280};\n'
        code += 'struct st7789v_data data = {.x_offset = 0, .y_offset = 20, .orientation = DISPLAY_ORIENTATION_NORMAL};\nstruct device dev = {.config = &config, .data = &data};\n' + body + '\n}\n'
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / 'zephyr').mkdir()
            (tmp / 'zephyr/kernel.h').write_text('')
            (tmp / 'test.c').write_text(code)
            build = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                                    '-I', str(tmp), '-I', str(ROOT / 'drivers/display'),
                                    str(tmp / 'test.c'), '-o', str(tmp / 'test')], capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            run = subprocess.run([str(tmp / 'test')], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_landscape_address_window_stays_aligned_after_repeated_flips(self):
        self.run_driver('''
for (int start = 1; start <= 3; start += 2) {
 for (int i = 0; i < 10; ++i) {
  enum display_orientation target = (i % 2) ? (4 - start) : start;
  assert(st7789v_set_orientation(&dev, target) == 0);
  assert(data.x_offset == 20 && data.y_offset == 0);
  assert(st7789v_set_mem_area(&dev, 0, 0, 280, 240) == 0);
  assert(columns[0] == 20 && columns[1] == 299);
  assert(rows[0] == 0 && rows[1] == 239);
 }
}
''')

    def test_failed_rotation_preserves_existing_write_offsets(self):
        self.run_driver('''
assert(st7789v_set_orientation(&dev, DISPLAY_ORIENTATION_ROTATED_90) == 0);
transport_result = -EIO;
assert(st7789v_set_orientation(&dev, DISPLAY_ORIENTATION_ROTATED_270) == -EIO);
assert(data.orientation == DISPLAY_ORIENTATION_ROTATED_90);
assert(data.x_offset == 20 && data.y_offset == 0);
transport_result = 0;
assert(st7789v_set_mem_area(&dev, 0, 0, 280, 240) == 0);
assert(columns[0] == 20 && rows[0] == 0);
''')

    def test_return_to_portrait_uses_original_offsets(self):
        self.run_driver('''
assert(st7789v_set_orientation(&dev, DISPLAY_ORIENTATION_ROTATED_90) == 0);
assert(st7789v_set_orientation(&dev, DISPLAY_ORIENTATION_NORMAL) == 0);
assert(data.x_offset == 0 && data.y_offset == 20);
assert(st7789v_set_mem_area(&dev, 0, 0, 240, 280) == 0);
assert(columns[0] == 0 && columns[1] == 239);
assert(rows[0] == 20 && rows[1] == 299);
''')
