#pragma once

#include <stdint.h>

/** Adjust brightness by delta (positive = brighter, negative = dimmer).
 *  Clamps result to [1, 100]. */
void prospector_adjust_brightness(int delta);

/** Get current brightness level (1-100). */
uint8_t prospector_get_brightness(void);
