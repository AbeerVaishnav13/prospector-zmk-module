#include "theme_cycle.h"

/* Preserve the previous bright shade for each layer name, without recoloring
 * other widgets or scheduling whole-screen refreshes. */
uint32_t operator_layer_text_color(uint8_t layer_index) {
    switch (layer_index) {
    case 0: /* home */
        return 0xFFFFFF;
    case 1: /* num_sym */
        return 0x00FF00;
    case 2: /* graphite */
        return 0xFF4646;
    case 3: /* media */
        return 0xFF7F00;
    case 4: /* nav */
    case 5: /* nav_lh */
        return 0x6262FF;
    case 6: /* lh */
    case 7: /* rh */
        return 0xFF34FF;
    case 8: /* mmv */
    case 9: /* msc */
        return 0x00FFFF;
    default:
        return 0xFFFF00;
    }
}
