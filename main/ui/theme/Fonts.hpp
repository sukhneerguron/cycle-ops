#pragma once
#include "lvgl.h"

namespace ui::theme {

/// Centralized font selection.
struct Fonts {
    static const lv_font_t* LargeNumber() { return &lv_font_montserrat_28; }
    static const lv_font_t* SmallLabel()  { return &lv_font_montserrat_10; }
};

} // namespace ui::theme
