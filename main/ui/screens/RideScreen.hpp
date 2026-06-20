#pragma once

#include "lvgl.h"

namespace ui { class SpeedWidget; }
namespace ui { class CadenceWidget; }

namespace ui {

/// RideScreen is the top-level LVGL screen for the ride view.
///
/// It composes and lays out widgets. It performs no calculations.
/// Call create() once during UI init while holding the LVGL mutex.
class RideScreen {
public:
    /// Build the screen and all child widgets, load it.
    /// Widgets are statically owned — caller passes references.
    /// Must be called while the LVGL mutex is held.
    static void create(SpeedWidget& speed, CadenceWidget& cadence);

private:
    RideScreen() = delete;
};

} // namespace ui
