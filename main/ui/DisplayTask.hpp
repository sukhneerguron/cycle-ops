#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "events/EventBus.hpp"
#include "models/RideModel.hpp"
#include "drivers/DisplayDriver.hpp"
#include "drivers/TouchDriver.hpp"

// Forward declare to avoid circular includes
namespace ui { class SpeedWidget; }
namespace ui { class CadenceWidget; }

namespace ui {

/// DisplayTask owns the LVGL event loop.
///
/// Responsibilities:
///   - Run lv_timer_handler() on a dedicated FreeRTOS task.
///   - Bridge system EventBus signals to direct widget update() calls,
///     keeping Services entirely decoupled from LVGL.
///
/// The Driver owns hardware. DisplayTask owns the LVGL runtime.
class DisplayTask {
public:
    /// @param driver      Hardware driver — provides lock()/unlock().
    /// @param model       Read-only access to the latest ride snapshot.
    /// @param event_bus   System event bus (RIDE_DATA_UPDATED, etc.).
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

    // Owned widget pointers — set during start() under the LVGL lock
    SpeedWidget*   speed_widget_{nullptr};
    CadenceWidget* cadence_widget_{nullptr};

    static constexpr uint32_t    kTaskStackDepth = 8192;
    static constexpr UBaseType_t kTaskPriority   = 4;
};

} // namespace ui
