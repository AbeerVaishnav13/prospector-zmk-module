#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <prospector/display_rotation.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static atomic_t pending_toggles;

static void rotation_work_handler(struct k_work *work) {
    (void)work;
    /* Work submissions may coalesce: two pending presses cancel, odd presses flip. */
    atomic_val_t toggles = atomic_set(&pending_toggles, 0);
    if (!(toggles & 1)) {
        return;
    }

    struct display_capabilities capabilities;
    display_get_capabilities(display, &capabilities);
    enum display_orientation target;
    switch (capabilities.current_orientation) {
    case DISPLAY_ORIENTATION_ROTATED_90:
        target = DISPLAY_ORIENTATION_ROTATED_270;
        break;
    case DISPLAY_ORIENTATION_ROTATED_270:
        target = DISPLAY_ORIENTATION_ROTATED_90;
        break;
    default:
        LOG_ERR("Cannot toggle non-landscape display orientation: %d",
                capabilities.current_orientation);
        return;
    }

    int ret = display_set_orientation(display, target);
    if (ret < 0) {
        LOG_ERR("Display rotation failed: %d", ret);
        return;
    }

    /* Both orientations have identical dimensions. Repaint once on the UI queue. */
    lv_obj_t *screen = lv_scr_act();
    if (screen) {
        lv_obj_invalidate(screen);
    }
}

K_WORK_DEFINE(rotation_work, rotation_work_handler);

int prospector_toggle_display_rotation(void) {
    if (!device_is_ready(display)) {
        return -ENODEV;
    }
    if (!zmk_display_is_initialized()) {
        return -EAGAIN;
    }

    atomic_inc(&pending_toggles);
    int ret = k_work_submit_to_queue(zmk_display_work_q(), &rotation_work);
    if (ret < 0) {
        atomic_dec(&pending_toggles);
        return ret;
    }
    return 0;
}

int disp_set_orientation(void) {
    if (!device_is_ready(display)) {
        return -EIO;
    }

#ifdef CONFIG_PROSPECTOR_ROTATE_DISPLAY_180
    int ret = display_set_orientation(display, DISPLAY_ORIENTATION_ROTATED_90);
#else
    int ret = display_set_orientation(display, DISPLAY_ORIENTATION_ROTATED_270);
#endif
    if (ret < 0) {
        LOG_ERR("Initial display rotation failed: %d", ret);
    }
    return ret;
}

SYS_INIT(disp_set_orientation, APPLICATION, 60);
