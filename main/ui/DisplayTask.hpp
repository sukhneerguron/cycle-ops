#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "events/EventBus.hpp"
#include "models/RideModel.hpp"
#include "drivers/DisplayDriver.hpp"
#include "drivers/TouchDriver.hpp"
#include "ui/components/SpeedWidget.hpp"
#include "ui/components/CadenceWidget.hpp"

namespace ui {

/// DisplayTask owns the LVGL event loop.
///
/// Responsibilities:
///   - Run lv_timer_handler() on a dedicated FreeRTOS task.
///   - Bridge system EventBus signals to direct widget update() calls,
///     keeping Services entirely decoupled from LVGL.
///
/// The Driver owns hardware. DisplayTask owns the LVGL runtime.
/// Widgets are statically owned — no heap allocation.
class DisplayTask {
public:
    DisplayTask(drivers::DisplayDriver& driver,
                drivers::TouchDriver& touch_driver,
                models::RideModel& model,
                events::EventBus& event_bus);
    ~DisplayTask();

    /// Spawns the FreeRTOS task and builds the initial screen.
    void start();

private:
    static void task_entry(void* arg);
    void run();

    drivers::DisplayDriver& driver_;
    drivers::TouchDriver&   touch_driver_;
    models::RideModel&      model_;
    events::EventBus&       event_bus_;
    TaskHandle_t            task_handle_{nullptr};
    lv_obj_t*               cursor_obj_{nullptr};

    // Statically owned widgets
    SpeedWidget   speed_widget_;
    CadenceWidget cadence_widget_;

    static constexpr uint32_t    kTaskStackDepth = 8192;
    static constexpr UBaseType_t kTaskPriority   = 4;
};

} // namespace ui
