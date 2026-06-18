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
class CadenceWidget {
public:
    /// Build and attach this widget to the given parent object.
    /// Must be called while the LVGL mutex is held.
    /// @returns a pointer to this widget so DisplayTask can call update().
    static CadenceWidget* create(lv_obj_t* parent);

    /// Refresh the displayed value from a new model snapshot.
    /// Must be called while the LVGL mutex is held.
    void update(const models::RideData& data);

private:
    explicit CadenceWidget(lv_obj_t* parent);

    lv_obj_t* container_{nullptr};
    lv_obj_t* cadence_label_{nullptr};
    lv_obj_t* unit_label_{nullptr};
};

} // namespace ui
