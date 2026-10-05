#define DT_DRV_COMPAT zmk_behavior_display_rotation_toggle

#include <errno.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <prospector/display_rotation.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_rotation_pressed(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    (void)binding;
    (void)event;
#if IS_ENABLED(CONFIG_SHIELD_PROSPECTOR_ADAPTER)
    int ret = prospector_toggle_display_rotation();
    return ret < 0 ? ret : ZMK_BEHAVIOR_OPAQUE;
#else
    /* Shared keymaps also instantiate this on halves without the display. */
    return -ENOTSUP;
#endif
}

static int on_rotation_released(struct zmk_behavior_binding *binding,
                                struct zmk_behavior_binding_event event) {
    (void)binding;
    (void)event;
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api rotation_driver_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = on_rotation_pressed,
    .binding_released = on_rotation_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define ROTATION_INST(n) \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rotation_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ROTATION_INST)

#endif
