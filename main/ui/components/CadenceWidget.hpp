#pragma once

#include "lvgl.h"
#include "models/RideModel.hpp"

namespace ui {

/// CadenceWidget is a pure LVGL micro-widget.
///
/// It owns its LVGL objects and exposes a single update() method.
/// DisplayTask calls update() (under the LVGL lock) whenever new
/// ride data arrives. No calculation is performed here — only
/// formatting for display.
///
/// Owned by DisplayTask as a static member — no heap allocation.
class CadenceWidget {
public:
    CadenceWidget() = default;

    /// Build LVGL objects under the given parent.
    /// Must be called while the LVGL mutex is held.
    void create(lv_obj_t* parent);

    /// Refresh the displayed value from a new model snapshot.
    /// Must be called while the LVGL mutex is held.
    void update(const models::RideData& data);

private:
    lv_obj_t* container_{nullptr};
    lv_obj_t* cadence_label_{nullptr};
    lv_obj_t* unit_label_{nullptr};
};

} // namespace ui
