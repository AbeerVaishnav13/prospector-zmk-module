#include "theme_cycle.h"

#include <zephyr/kernel.h>

#define THEME_CYCLE_TICK_MS   500
#define THEME_CYCLE_PERIOD_MS (60 * 1000)
#define THEME_CYCLE_STEPS     (THEME_CYCLE_PERIOD_MS / THEME_CYCLE_TICK_MS)
#define THEME_CYCLE_MAX_CBS   8
#define THEME_START_HUE_X100  12000

uint32_t theme_dyn_dark   = 0x003B00;
uint32_t theme_dyn_med    = 0x008F00;
uint32_t theme_dyn_bright = 0x00FF00;

static theme_refresh_cb_t refresh_cbs[THEME_CYCLE_MAX_CBS];
static int refresh_cb_count = 0;
static int32_t hue_x100 = THEME_START_HUE_X100;
static struct k_work_delayable theme_work;

static uint32_t hsv_to_rgb(int32_t h_x100, uint8_t v) {
    while (h_x100 < 0) h_x100 += 36000;
    while (h_x100 >= 36000) h_x100 -= 36000;

    int32_t sector = h_x100 / 6000;
    int32_t frac = h_x100 - sector * 6000;

    uint32_t vv = v;
    uint32_t p = 0;
    uint32_t q = (vv * (6000 - frac)) / 6000;
    uint32_t t = (vv * frac) / 6000;

    uint32_t r, g, b;
    switch (sector) {
    case 0: r = vv; g = t;  b = p;  break;
    case 1: r = q;  g = vv; b = p;  break;
    case 2: r = p;  g = vv; b = t;  break;
    case 3: r = p;  g = q;  b = vv; break;
    case 4: r = t;  g = p;  b = vv; break;
    default: r = vv; g = p; b = q;  break;
    }

    return (r << 16) | (g << 8) | b;
}

static void theme_work_handler(struct k_work *work) {
    hue_x100 += 36000 / THEME_CYCLE_STEPS;
    if (hue_x100 >= 36000) hue_x100 -= 36000;

    theme_dyn_dark   = hsv_to_rgb(hue_x100, 59);
    theme_dyn_med    = hsv_to_rgb(hue_x100, 143);
    theme_dyn_bright = hsv_to_rgb(hue_x100, 255);

    for (int i = 0; i < refresh_cb_count; i++) {
        if (refresh_cbs[i]) {
            refresh_cbs[i]();
        }
    }

    k_work_schedule(&theme_work, K_MSEC(THEME_CYCLE_TICK_MS));
}

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
    k_work_init_delayable(&theme_work, theme_work_handler);
    k_work_schedule(&theme_work, K_MSEC(THEME_CYCLE_TICK_MS));
}
