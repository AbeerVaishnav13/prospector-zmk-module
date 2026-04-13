#include "theme_cycle.h"

#include <zephyr/kernel.h>

#define THEME_CYCLE_TICK_MS   500
#define THEME_CYCLE_PERIOD_MS (5 * 60 * 1000)
#define THEME_CYCLE_STEPS     (THEME_CYCLE_PERIOD_MS / THEME_CYCLE_TICK_MS)
#define THEME_CYCLE_MAX_CBS   8
#define THEME_START_HUE_X100  12000

#define LUMA_FLOOR_DARK   150000
#define LUMA_FLOOR_MED    500000
#define LUMA_FLOOR_BRIGHT 1100000

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

static uint32_t hue_tier_color(int32_t h_x100, uint8_t v, int32_t floor) {
    uint32_t rgb = hsv_to_rgb(h_x100, v);
    int32_t r = (rgb >> 16) & 0xFF;
    int32_t g = (rgb >> 8) & 0xFF;
    int32_t b = rgb & 0xFF;
    int32_t luma = 2126 * r + 7152 * g + 722 * b;

    if (luma >= floor) {
        return rgb;
    }

    if (luma > 0) {
        int32_t v_new = ((int32_t)v * floor) / luma;
        if (v_new <= 255) {
            return hsv_to_rgb(h_x100, (uint8_t)v_new);
        }
    }

    uint32_t rgb255 = hsv_to_rgb(h_x100, 255);
    int32_t r255 = (rgb255 >> 16) & 0xFF;
    int32_t g255 = (rgb255 >> 8) & 0xFF;
    int32_t b255 = rgb255 & 0xFF;
    int32_t luma255 = 2126 * r255 + 7152 * g255 + 722 * b255;
    int32_t w_num = floor - luma255;
    int32_t w_den = 2550000 - luma255;
    if (w_den <= 0 || w_num <= 0) {
        return rgb255;
    }
    int32_t rr = r255 + (w_num * (255 - r255)) / w_den;
    int32_t gg = g255 + (w_num * (255 - g255)) / w_den;
    int32_t bb = b255 + (w_num * (255 - b255)) / w_den;
    if (rr > 255) rr = 255;
    if (gg > 255) gg = 255;
    if (bb > 255) bb = 255;
    return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
}

static void theme_work_handler(struct k_work *work) {
    hue_x100 += 36000 / THEME_CYCLE_STEPS;
    if (hue_x100 >= 36000) hue_x100 -= 36000;

    theme_dyn_dark   = hue_tier_color(hue_x100, 59,  LUMA_FLOOR_DARK);
    theme_dyn_med    = hue_tier_color(hue_x100, 143, LUMA_FLOOR_MED);
    theme_dyn_bright = hue_tier_color(hue_x100, 255, LUMA_FLOOR_BRIGHT);

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
