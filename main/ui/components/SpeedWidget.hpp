#pragma once

#include "lvgl.h"
#include "models/RideModel.hpp"

namespace ui {

/// SpeedWidget is a pure LVGL micro-widget.
///
/// It owns its LVGL objects and exposes a single update() method.
/// DisplayTask calls update() (under the LVGL lock) whenever new
/// ride data arrives. No calculation is performed here — only
/// formatting for display.
///
/// Owned by DisplayTask as a static member — no heap allocation.
class SpeedWidget {
public:
    SpeedWidget() = default;

    /// Build LVGL objects under the given parent.
    /// Must be called while the LVGL mutex is held.
    void create(lv_obj_t* parent, lv_subject_t* ride_subject);

    /// LVGL observer callback for ride data updates.
    static void onRideDataObserved(lv_observer_t* observer, lv_subject_t* subject);

private:
    lv_obj_t* container_{nullptr};
    lv_obj_t* speed_label_{nullptr};
    lv_obj_t* unit_label_{nullptr};
};

} // namespace ui
