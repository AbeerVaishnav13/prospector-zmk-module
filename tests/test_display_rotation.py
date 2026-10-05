"""Host execution tests using Zephyr/ZMK/LVGL interface doubles, not a firmware build."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
STUBS = r'''
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>
struct device { int unused; };
static struct device mock_device;
#define DT_CHOSEN(x) 0
#define DEVICE_DT_GET(x) (&mock_device)
#define SYS_INIT(a,b,c)
#define LOG_MODULE_DECLARE(a,b)
static int error_logs;
#define LOG_ERR(...) (++error_logs)
#define IS_ENABLED(x) (x)
#define DT_HAS_COMPAT_STATUS_OKAY(x) 1
struct k_work { void (*handler)(struct k_work *); };
struct k_work_q { int unused; };
static struct k_work_q ui_queue;
static struct k_work *queued_work;
static int submit_result;
#define K_WORK_DEFINE(name,fn) static struct k_work name = {fn}
static int k_work_submit_to_queue(struct k_work_q *queue, struct k_work *work) {
    assert(queue == &ui_queue);
    if (submit_result < 0) return submit_result;
    queued_work = work;
    return submit_result;
}
typedef long atomic_t;
typedef long atomic_val_t;
static atomic_val_t atomic_set(atomic_t *p, atomic_val_t n) { long old=*p; *p=n; return old; }
static void atomic_inc(atomic_t *p) { ++*p; }
static void atomic_dec(atomic_t *p) { --*p; }
enum display_orientation { DISPLAY_ORIENTATION_NORMAL, DISPLAY_ORIENTATION_ROTATED_90,
                           DISPLAY_ORIENTATION_ROTATED_180, DISPLAY_ORIENTATION_ROTATED_270 };
struct display_capabilities { enum display_orientation current_orientation; };
static enum display_orientation orientation;
static bool ready = true, initialized = true;
static int driver_result, driver_calls, invalidations;
static bool device_is_ready(const struct device *d) { (void)d; return ready; }
static int display_set_orientation(const struct device *d, enum display_orientation o) {
    (void)d; ++driver_calls;
    if (driver_result < 0) return driver_result;
    orientation=o; return 0;
}
static void display_get_capabilities(const struct device *d, struct display_capabilities *c) {
    (void)d; c->current_orientation=orientation;
}
static bool zmk_display_is_initialized(void) { return initialized; }
static struct k_work_q *zmk_display_work_q(void) { return &ui_queue; }
typedef struct { int unused; } lv_obj_t;
static lv_obj_t screen;
static lv_obj_t *lv_scr_act(void) { return &screen; }
static void lv_obj_invalidate(lv_obj_t *obj) { assert(obj == &screen); ++invalidations; }
struct zmk_behavior_binding { int unused; };
struct zmk_behavior_binding_event { int unused; };
#define ZMK_BEHAVIOR_OPAQUE 0
#define BEHAVIOR_LOCALITY_CENTRAL 0
struct behavior_driver_api {
    int locality;
    int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
    int (*get_parameter_metadata)(void);
};
static int zmk_behavior_get_empty_param_metadata(void) { return 0; }
#define DT_INST_FOREACH_STATUS_OKAY(fn) fn(0)
#define BEHAVIOR_DT_INST_DEFINE(n,init,pm,data,cfg,level,prio,api) \
    static const struct behavior_driver_api *registered_api = api
'''
HARNESS = r'''
static void drain(void) {
    assert(queued_work != NULL);
    struct k_work *work = queued_work;
    queued_work = NULL;
    work->handler(work);
}
int main(void) {
#if CONFIG_SHIELD_PROSPECTOR_ADAPTER
    assert(disp_set_orientation() == 0);
#ifdef CONFIG_PROSPECTOR_ROTATE_DISPLAY_180
    assert(orientation == DISPLAY_ORIENTATION_ROTATED_90);
#else
    assert(orientation == DISPLAY_ORIENTATION_ROTATED_270);
#endif
    enum display_orientation initial = orientation;
    struct zmk_behavior_binding binding = {0};
    struct zmk_behavior_binding_event event = {0};
    assert(registered_api->locality == BEHAVIOR_LOCALITY_CENTRAL);
    assert(registered_api->binding_pressed(&binding, event) == ZMK_BEHAVIOR_OPAQUE);
    assert(orientation == initial); /* No driver/LVGL call from key event thread. */
    assert(registered_api->binding_released(&binding, event) == ZMK_BEHAVIOR_OPAQUE);
    drain(); assert(orientation != initial); assert(invalidations == 1);
    assert(prospector_toggle_display_rotation() == 0);
    drain(); assert(orientation == initial); assert(invalidations == 2);
    int before = driver_calls;
    assert(prospector_toggle_display_rotation() == 0);
    assert(prospector_toggle_display_rotation() == 0);
    drain(); assert(orientation == initial); assert(driver_calls == before);
    for (int i=0; i<3; ++i) assert(prospector_toggle_display_rotation() == 0);
    drain(); assert(orientation != initial);
    ready=false; assert(prospector_toggle_display_rotation() == -ENODEV);
    assert(disp_set_orientation() == -EIO); ready=true;
    initialized=false; assert(prospector_toggle_display_rotation() == -EAGAIN);
    initialized=true;
    submit_result=-EBUSY;
    assert(registered_api->binding_pressed(&binding, event) == -EBUSY);
    submit_result=0;
    enum display_orientation previous=orientation;
    driver_result=-EIO;
    assert(prospector_toggle_display_rotation() == 0);
    before=invalidations;
    drain(); assert(orientation == previous); assert(invalidations == before);
    assert(error_logs == 1); driver_result=0;
    assert(prospector_toggle_display_rotation() == 0);
    drain(); assert(orientation != previous);
    orientation=DISPLAY_ORIENTATION_NORMAL;
    assert(prospector_toggle_display_rotation() == 0);
    drain(); assert(orientation == DISPLAY_ORIENTATION_NORMAL);
    assert(error_logs == 2);
    /* Restart restores config default, not the runtime toggle. */
    assert(disp_set_orientation() == 0); assert(orientation == initial);
#if CONFIG_ZMK_BEHAVIOR_METADATA
    assert(registered_api->get_parameter_metadata == zmk_behavior_get_empty_param_metadata);
#endif
#else
    struct zmk_behavior_binding binding = {0};
    struct zmk_behavior_binding_event event = {0};
    assert(registered_api->binding_pressed(&binding,event) == -ENOTSUP);
    assert(registered_api->binding_released(&binding,event) == ZMK_BEHAVIOR_OPAQUE);
#endif
    return 0;
}
'''


class DisplayRotationTest(unittest.TestCase):
    def test_compiled_toggle_and_behavior(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            (p / "mock.h").write_text(STUBS)
            for header in ["zephyr/kernel.h", "zephyr/device.h", "zephyr/drivers/display.h",
                           "zephyr/sys/atomic.h", "zephyr/logging/log.h", "lvgl.h",
                           "zmk/display.h", "drivers/behavior.h", "zmk/behavior.h"]:
                path = p / header
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "mock.h"\n')
            rotation = ROOT / "boards/shields/prospector_adapter/src/display_rotate_init.c"
            behavior = ROOT / "src/behaviors/behavior_display_rotation.c"
            source = p / "test.c"
            for shield, rotated, metadata in [(1, 0, 0), (1, 1, 1), (0, 0, 0)]:
                with self.subTest(shield=shield, rotated=rotated, metadata=metadata):
                    includes = '#include "mock.h"\n'
                    if shield:
                        includes += f'#include "{rotation}"\n'
                    includes += f'#include "{behavior}"\n'
                    source.write_text(includes + HARNESS)
                    cmd = ["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                           "-Wno-unused-function", "-Wno-unused-variable",
                           f"-I{p}", f"-I{ROOT / 'include'}",
                           f"-DCONFIG_SHIELD_PROSPECTOR_ADAPTER={shield}",
                           f"-DCONFIG_ZMK_BEHAVIOR_METADATA={metadata}"]
                    if rotated:
                        cmd.append("-DCONFIG_PROSPECTOR_ROTATE_DISPLAY_180=1")
                    executable = p / "rotation-test"
                    subprocess.run(cmd + [str(source), "-o", str(executable)], check=True)
                    subprocess.run([str(executable)], check=True)

    def test_devicetree_and_build_integration(self):
        import yaml
        binding = yaml.safe_load((ROOT / "dts/bindings/behaviors/zmk,behavior-display-rotation-toggle.yaml").read_text())
        self.assertEqual(binding["include"], "zero_param.yaml")
        dtsi = (ROOT / "dts/behaviors/display_rotation.dtsi").read_text()
        self.assertIn(binding["compatible"], dtsi)
        self.assertIn("rot_disp: rot_disp", dtsi)
        self.assertIn("#binding-cells = <0>", dtsi)
        cmake = (ROOT / "CMakeLists.txt").read_text()
        self.assertIn("CONFIG_DT_HAS_ZMK_BEHAVIOR_DISPLAY_ROTATION_TOGGLE_ENABLED", cmake)
        self.assertIn("src/behaviors/behavior_display_rotation.c", cmake)


if __name__ == "__main__":
    unittest.main()
