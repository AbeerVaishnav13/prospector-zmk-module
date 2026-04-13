#pragma once

#include <stdint.h>

extern uint32_t theme_dyn_dark;
extern uint32_t theme_dyn_med;
extern uint32_t theme_dyn_bright;

#define DISPLAY_COLOR_MOD_ACTIVE         theme_dyn_bright
#define DISPLAY_COLOR_MOD_INACTIVE       theme_dyn_dark
#define DISPLAY_COLOR_MOD_SEPARATOR      theme_dyn_dark
#define DISPLAY_COLOR_MOD_CAPS_WORD      0xCC3333

#define DISPLAY_COLOR_WPM_BAR_ACTIVE     theme_dyn_med
#define DISPLAY_COLOR_WPM_BAR_INACTIVE   0x0D0208
#define DISPLAY_COLOR_WPM_TEXT           theme_dyn_med
#define DISPLAY_COLOR_WPM_PEAK           0x6B2020

#define DISPLAY_COLOR_LAYER_TEXT         theme_dyn_bright
#define DISPLAY_COLOR_LAYER_DOT_ACTIVE   theme_dyn_med
#define DISPLAY_COLOR_LAYER_DOT_INACTIVE theme_dyn_dark

#define DISPLAY_COLOR_BATTERY_FILL       theme_dyn_med
#define DISPLAY_COLOR_BATTERY_RING       theme_dyn_dark
#define DISPLAY_COLOR_BATTERY_BG         0x0D0208
#define DISPLAY_COLOR_BATTERY_LABEL      theme_dyn_med

#define DISPLAY_COLOR_BATTERY_DISCONNECTED_FILL  0xCC3333
#define DISPLAY_COLOR_BATTERY_DISCONNECTED_RING  0x6B2020
#define DISPLAY_COLOR_BATTERY_DISCONNECTED_LABEL 0xCC3333

#define DISPLAY_COLOR_BATTERY_LOW_FILL   0xCC3333
#define DISPLAY_COLOR_BATTERY_LOW_RING   0x6B2020

#define DISPLAY_COLOR_USB_ACTIVE_BG        0xCC3333
#define DISPLAY_COLOR_USB_INACTIVE_BG      0x6B2020
#define DISPLAY_COLOR_BLE_ACTIVE_BG        theme_dyn_med
#define DISPLAY_COLOR_BLE_INACTIVE_BG      theme_dyn_dark
#define DISPLAY_COLOR_OUTPUT_ACTIVE_TEXT   0x000000
#define DISPLAY_COLOR_OUTPUT_INACTIVE_TEXT theme_dyn_dark

#define DISPLAY_COLOR_SLOT_TEXT            0x000000
#define DISPLAY_COLOR_SLOT_UNPAIRED        0x6B2020
#define DISPLAY_COLOR_SLOT_PAIRED          theme_dyn_dark
#define DISPLAY_COLOR_SLOT_CONNECTED_BG    theme_dyn_med
#define DISPLAY_COLOR_SLOT_DISCONNECTED_BG 0xCC3333
