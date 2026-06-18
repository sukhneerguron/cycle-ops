#pragma once

#include "lvgl.h"

// Forward declare to avoid pulling in all widget headers here
namespace ui { class SpeedWidget; }
namespace ui { class CadenceWidget; }

namespace ui {

/// RideScreen bundles the widgets returned to the caller so DisplayTask
/// can hold both pointers and call update() on each.
struct RideWidgets {
    SpeedWidget*   speed{nullptr};
    CadenceWidget* cadence{nullptr};
};

/// RideScreen is the top-level LVGL screen for the ride view.
///
/// It composes and lays out widgets. It performs no calculations.
/// Call create() once during UI init while holding the LVGL mutex.
class RideScreen {
public:
    /// Build the screen and all child widgets, load it, and return the
    /// widget pointers so DisplayTask can push updates to them.
    /// Must be called while the LVGL mutex is held.
    static RideWidgets create();

private:
    RideScreen() = delete;
};

} // namespace ui
