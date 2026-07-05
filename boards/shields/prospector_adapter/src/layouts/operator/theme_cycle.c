#include "theme_cycle.h"

#include <stdbool.h>
#include <zmk/keymap.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>

#define THEME_CYCLE_MAX_CBS 8

struct operator_theme_colors {
    uint32_t bg;
    uint32_t dark;
    uint32_t med;
    uint32_t bright;
};

/*
 * The four shades use the same HSV value/luma tiers as the old cycling theme:
 * bg v=13, dark v=59 with luma floor, med v=143 with luma floor, bright v=255
 * with luma floor. That keeps each predefined layer color visually close to
 * the previous color-wheel shades, only without time-based hue cycling.
 */
static const struct operator_theme_colors operator_theme_yellow  = {0x0D0D00, 0x3B3B00, 0x8F8F00, 0xFFFF00};
static const struct operator_theme_colors operator_theme_green   = {0x000D00, 0x003B00, 0x008F00, 0x00FF00};
static const struct operator_theme_colors operator_theme_orange  = {0x0D0600, 0x3B1D00, 0x8F4700, 0xFF7F00};
static const struct operator_theme_colors operator_theme_blue    = {0x00000D, 0x0000A6, 0x2222FF, 0x6262FF};
static const struct operator_theme_colors operator_theme_red     = {0x0D0000, 0x3B0000, 0xEB0000, 0xFF4646};
static const struct operator_theme_colors operator_theme_magenta = {0x0D000D, 0x3B003B, 0xAF00AF, 0xFF34FF};
static const struct operator_theme_colors operator_theme_cyan    = {0x000D0D, 0x003B3B, 0x008F8F, 0x00FFFF};

uint32_t theme_dyn_bg     = 0x000D00;
uint32_t theme_dyn_dark   = 0x003B00;
uint32_t theme_dyn_med    = 0x008F00;
uint32_t theme_dyn_bright = 0x00FF00;

static theme_refresh_cb_t refresh_cbs[THEME_CYCLE_MAX_CBS];
static int refresh_cb_count = 0;

static const struct operator_theme_colors *theme_for_layer_index(uint8_t layer_index) {
    switch (layer_index) {
    case 0:  /* home */
        return &operator_theme_yellow;
    case 1:  /* num_sym */
        return &operator_theme_green;
    case 2:  /* graphite */
        return &operator_theme_red;
    case 3:  /* media */
        return &operator_theme_orange;
    case 4:  /* nav */
    case 5:  /* nav_lh */
        return &operator_theme_blue;
    case 6:  /* lh */
    case 7:  /* rh */
        return &operator_theme_magenta;
    case 8:  /* mmv */
    case 9:  /* msc */
        return &operator_theme_cyan;
    default:
        return &operator_theme_yellow;
    }
}

static void apply_theme(const struct operator_theme_colors *theme) {
    theme_dyn_bg = theme->bg;
    theme_dyn_dark = theme->dark;
    theme_dyn_med = theme->med;
    theme_dyn_bright = theme->bright;
}

static void notify_theme_refresh(void) {
    for (int i = 0; i < refresh_cb_count; i++) {
        if (refresh_cbs[i]) {
            refresh_cbs[i]();
        }
    }
}

static void update_theme_for_active_layer(void) {
    apply_theme(theme_for_layer_index(zmk_keymap_highest_layer_active()));
}

static int operator_theme_layer_listener(const zmk_event_t *eh) {
    (void)eh;
    update_theme_for_active_layer();
    notify_theme_refresh();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(operator_theme_layer, operator_theme_layer_listener);
ZMK_SUBSCRIPTION(operator_theme_layer, zmk_layer_state_changed);

void theme_cycle_register_refresh(theme_refresh_cb_t cb) {
    if (refresh_cb_count >= THEME_CYCLE_MAX_CBS) return;
    for (int i = 0; i < refresh_cb_count; i++) {
        if (refresh_cbs[i] == cb) return;
    }
    refresh_cbs[refresh_cb_count++] = cb;
}

void theme_cycle_start(void) {
    static bool started = false;
    if (started) return;
    started = true;
    update_theme_for_active_layer();
    notify_theme_refresh();
}
