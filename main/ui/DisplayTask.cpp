#include "DisplayTask.hpp"
#include "ui/screens/RideScreen.hpp"
#include "ui/theme/Colors.hpp"
#include "lvgl.h"
#include "esp_log.h"

namespace ui {

static const char* TAG = "DisplayTask";

DisplayTask::DisplayTask(drivers::DisplayDriver& driver,
                         drivers::TouchDriver& touch_driver,
                         models::RideModel& model,
                         events::EventBus& event_bus)
    : driver_(driver), touch_driver_(touch_driver), model_(model), event_bus_(event_bus) {
}

DisplayTask::~DisplayTask() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
}

void DisplayTask::start() {
    ESP_LOGI(TAG, "Building initial UI");

    // Build the screen while holding the LVGL lock
    driver_.lock();
    RideScreen::create(speed_widget_, cadence_widget_);

    // Create touch cursor on the active screen
    lv_obj_t* cursor = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cursor, 15, 15);
    lv_obj_set_style_radius(cursor, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cursor, theme::Colors::Cursor(), 0);
    lv_obj_set_style_bg_opa(cursor, LV_OPA_70, 0);
    lv_obj_set_style_border_width(cursor, 0, 0);
    lv_obj_clear_flag(cursor, LV_OBJ_FLAG_CLICKABLE);
    lv_indev_set_cursor(touch_driver_.get_indev(), cursor);
    cursor_obj_ = cursor;

    driver_.unlock();

    ESP_LOGI(TAG, "Starting LVGL task");
    xTaskCreate(task_entry, "DisplayTask", kTaskStackDepth, this, kTaskPriority, &task_handle_);
}

void DisplayTask::task_entry(void* arg) {
    auto* self = static_cast<DisplayTask*>(arg);
    self->run();
}

void DisplayTask::run() {
    constexpr EventBits_t kWakeEvents = 
        events::EventBus::RIDE_DATA_UPDATED | events::EventBus::TOUCH_INTERRUPT;

    while (true) {
        // Run LVGL timers / render cycle under the lock
        driver_.lock();
        uint32_t next_delay_ms = lv_timer_handler();
        driver_.unlock();

        // Cap the idle delay so we stay responsive to events
        if (next_delay_ms > 10) {
            next_delay_ms = 10;
        }

        // Wait for new ride data or touch interrupt — block for at most next_delay_ms
        EventBits_t bits = event_bus_.waitAny(
            kWakeEvents,
            pdMS_TO_TICKS(next_delay_ms)
        );

        if (bits & events::EventBus::TOUCH_INTERRUPT) {
            event_bus_.clear(events::EventBus::TOUCH_INTERRUPT);
            // break out of waiting and let lv_timer_handler() wakeup and handle the touch
        }

        if (bits & events::EventBus::RIDE_DATA_UPDATED) {
            event_bus_.clear(events::EventBus::RIDE_DATA_UPDATED);

            // Snapshot the model and push to all widgets under the LVGL lock
            models::RideData data = model_.get();

            driver_.lock();
            speed_widget_.update(data);
            cadence_widget_.update(data);
            driver_.unlock();
        }
    }
}

} // namespace ui
