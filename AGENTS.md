# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

## Overview

This is a ZMK firmware module for the [Prospector](https://github.com/carrefinho/prospector) keyboard — a custom split keyboard with a dongle as BLE central. The module provides four selectable LVGL-based status screen layouts for the dongle's ST7789V display (240×280, driven via SPI).

**This branch** (`feat/new-status-screens`) targets **ZMK main (Zephyr 4.1)** and uses board target `xiao_ble//zmk`.

## Building

There is no standalone build. This module is consumed by a ZMK keyboard config repo via `west`. Typical local build from within a ZMK workspace:

```sh
west build -s zmk/app -b xiao_ble//zmk -- \
  -DSHIELD="[YOUR_KEYBOARD]_dongle prospector_adapter" \
  -DZMK_EXTRA_MODULES="/path/to/prospector-zmk-module"
```

Or with the module declared in `config/west.yml` (see README):

```sh
west build -s zmk/app -b xiao_ble//zmk -- -DSHIELD="[shield]_dongle prospector_adapter"
```

If you get a `region 'RAM' overflowed` error, add `CONFIG_LV_Z_VDB_SIZE=25` to the keyboard's `.conf` file.

There are no tests or lint scripts in this repo.

## Architecture

### Module registration

- `zephyr/module.yml` — declares this as a Zephyr/ZMK module; sets `board_root: .`, `dts_root: .`, and depends on `lvgl`
- `CMakeLists.txt` (root) — conditionally compiles (when `CONFIG_SHIELD_PROSPECTOR_ADAPTER` is set): custom display driver from `drivers/display/`, custom event sources, the BT connection observer, and shadows ZMK's built-in `behavior_caps_word.c` to add caps word event emission. Also compiles `behavior_brightness.c` when `CONFIG_PROSPECTOR_BRIGHTNESS_MANUAL` is set
- `Kconfig` — all `CONFIG_PROSPECTOR_*` options (layout choice, modifier display/order/OS style, brightness mode, display rotation, layer name case, etc.)
- `dts/bindings/zmk,prospector-theme.yaml` — DTS binding for the `zmk,prospector-theme` compatible node used by Radii color theming
- `dts/behaviors/brightness.dtsi` — includable DTS fragment defining `&inc_bri` / `&dec_bri` keyboard-controlled brightness behaviors
- `drivers/display/` — custom ST7789V display driver (`display_st7789v.c`/`.h`)

### Shield: `boards/shields/prospector_adapter/`

- `prospector_adapter.overlay` — sets `zephyr,display = &st7789`; defines four built-in color theme nodes (green, blue, red, purple) for use with the Radii layout's DTS theming
- `boards/xiao_ble_zmk.overlay` — full hardware wiring: ST7789V via SPI3, APDS9960 ambient light sensor via I2C0, PWM backlight on PWM1
- `CMakeLists.txt` — compiles all font sources, `brightness.c`, `display_rotate_init.c`, conditionally `modifier_order.c` (when `CONFIG_PROSPECTOR_SHOW_MODIFIERS`), and the selected layout's sources (excluding `status_screen.c`, which is `#include`-d directly by `custom_status_screen.c`)
- `src/custom_status_screen.c` — the ZMK display entry point (`zmk_display_status_screen()`); selects the layout's `status_screen.c` via preprocessor `#include` based on `CONFIG_PROSPECTOR_STATUS_SCREEN_*`
- `src/brightness.c` — backlight control via PWM: auto mode with APDS9960 ALS polling thread and PWM fade (`CONFIG_PROSPECTOR_BRIGHTNESS_AUTO`), fixed brightness (`CONFIG_PROSPECTOR_BRIGHTNESS_FIXED`), or manual keyboard-controlled mode (`CONFIG_PROSPECTOR_BRIGHTNESS_MANUAL`) using `&inc_bri` / `&dec_bri` behaviors
- `src/modifier_order.c` — parses `CONFIG_PROSPECTOR_MODIFIER_ORDER` (e.g. `"GACS"`) at init time and exposes `modifier_order_get()`, symbol/text helpers, and OS style queries used by all layouts' modifier widgets

### Layouts (`src/layouts/`)

Four layouts, each a self-contained directory with its own widgets and a `status_screen.c`:

| Layout | Config | Key widgets |
|--------|--------|-------------|
| `classic` | `CONFIG_PROSPECTOR_STATUS_SCREEN_CLASSIC` (default) | `layer_roller`, `battery_bar`, `modifier_indicator`, `output` |
| `radii` | `CONFIG_PROSPECTOR_STATUS_SCREEN_RADII` | `layer_indicator`, `battery_circles`, `modifier_indicator`, `output`; DTS color theme |
| `field` | `CONFIG_PROSPECTOR_STATUS_SCREEN_FIELD` | `layer_label`, `battery_label`, `line_segments` (WPM animation), `modifier_indicator`, `output` |
| `operator` | `CONFIG_PROSPECTOR_STATUS_SCREEN_OPERATOR` | `layer_display`, `battery_circles`, `wpm_meter`, `modifier_indicator`, `output`; hardcoded color theme via `display_colors.h` |

Each layout's `output` widget handles `zmk_endpoint_changed` and `zmk_ble_active_profile_changed` events, showing the active BLE profile and connection state. The operator layout's output widget also colors the active profile slot by BLE connection state (pastel green = paired+connected, pastel red = paired+disconnected, pastel blue = unpaired) when BLE transport is active. Classic/field layouts coordinate with the battery widget's "compact" mode (battery bar collapses to make room for the output overlay, then restores after a 3-second timeout).

### Widget event pattern

All display widgets follow ZMK's pattern:
```c
ZMK_DISPLAY_WIDGET_LISTENER(name, state_struct, update_cb, get_state_fn)
ZMK_SUBSCRIPTION(name, zmk_event_type);
```

### Custom events

Two custom ZMK events extend ZMK's built-in event system:

- **`zmk_split_central_status_changed`** (`include/zmk/events/`, `src/events/`, `src/split/bluetooth/central_status_changed_observer.c`) — fired when a BLE peripheral connects or disconnects. Carries `slot` (peripheral index, matching the pairing order) and `connected`. The observer registers `bt_conn_cb` callbacks independently of ZMK's own split central machinery.

- **`zmk_caps_word_state_changed`** (`include/zmk/events/`, `src/events/`) — fired by a replacement `behavior_caps_word.c` (in `src/behaviors/`) that shadows ZMK's built-in via CMake `HEADER_FILE_ONLY` trick, adding event emission on caps word activate/deactivate.

### Color theming

**Radii** reads colors from a DTS node with `compatible = "zmk,prospector-theme"` via its `display_colors.h`. The active theme is resolved via `DT_CHOSEN(zmk_prospector_theme)`, falling back to the built-in `prospector_blue_theme` node defined in the overlay. Four themes are provided (green, blue, red, purple). To customize, define a node in your keyboard's overlay and set `chosen { zmk-prospector-theme = &your_theme; }`.

**Operator** uses hardcoded colors defined in its own `display_colors.h` (not DTS-driven). Colors are organized by widget: modifiers, WPM meter, layer display, battery circles, and output slot states.

### Fonts and symbols

Custom bitmap fonts live in `src/fonts/` (FoundryGridnik `FG_*`, FRAC `FR_*`, DINish, PP Formula `PPF_*`, Symbols). Declared in `include/fonts.h`. Custom symbol glyphs (BLE wave, shift, command, option, etc.) are in `include/symbols.h` with glyph data in `src/fonts/Symbols_*.c`.
