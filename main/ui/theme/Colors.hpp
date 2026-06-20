#pragma once
#include "lvgl.h"

namespace ui::theme {

/// Centralized color palette.
/// Changing themes should not require modifying widgets.
struct Colors {
  static constexpr lv_color_t Background() { return lv_color_white(); }
  static constexpr lv_color_t PrimaryText() { return lv_color_black(); }
  static constexpr lv_color_t SecondaryText() {
    return lv_color_make(0xAA, 0xAA, 0xAA);
  }
  static constexpr lv_color_t Cursor() { return lv_color_hex(0x0000FF); }
};

} // namespace ui::theme
