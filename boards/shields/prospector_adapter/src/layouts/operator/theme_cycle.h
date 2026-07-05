#pragma once

#include <stdint.h>

extern uint32_t theme_dyn_bg;
extern uint32_t theme_dyn_dark;
extern uint32_t theme_dyn_med;
extern uint32_t theme_dyn_bright;

typedef void (*theme_refresh_cb_t)(void);

void theme_cycle_register_refresh(theme_refresh_cb_t cb);
void theme_cycle_start(void);
