#include "DisplayConsumer.hpp"

namespace consumers {

DisplayConsumer::DisplayConsumer(models::RideModel& ride_model, drivers::DisplayDriver& display_driver)
    : ride_model_(ride_model), display_driver_(display_driver) {}

DisplayConsumer::~DisplayConsumer() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
}

void DisplayConsumer::start() {
    display_driver_.lock();
    create_ui();
    display_driver_.unlock();

    xTaskCreate(consumer_task, "display_consumer", 4096, this, 3, &task_handle_);
}

void DisplayConsumer::create_ui() {
    screen_ = lv_screen_active();
    lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);
    lv_obj_set_style_text_color(screen_, lv_color_white(), 0);

    // RPM Label
    rpm_label_ = lv_label_create(screen_);
    lv_obj_align(rpm_label_, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(rpm_label_, "RPM: 0");

    // Cadence Label
    cadence_label_ = lv_label_create(screen_);
    lv_obj_align(cadence_label_, LV_ALIGN_LEFT_MID, 5, 0);
    lv_label_set_text(cadence_label_, "Cad: 0");

    // Wheel Revs Label
    revs_label_ = lv_label_create(screen_);
    lv_obj_align(revs_label_, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_label_set_text(revs_label_, "Revs: 0");
}

void DisplayConsumer::consumer_task(void* arg) {
    auto* consumer = static_cast<DisplayConsumer*>(arg);
    while (true) {
        consumer->update_ui();
        vTaskDelay(pdMS_TO_TICKS(500)); // Update twice a second
    }
}

void DisplayConsumer::update_ui() {
    models::RideData data = ride_model_.get();

    display_driver_.lock();
    if (rpm_label_) lv_label_set_text_fmt(rpm_label_, "RPM: %u", data.current_rpm);
    if (cadence_label_) lv_label_set_text_fmt(cadence_label_, "Cad: %u", data.current_cadence);
    if (revs_label_) lv_label_set_text_fmt(revs_label_, "Revs: %lu", (unsigned long)data.total_wheel_revolutions);
    display_driver_.unlock();
}

} // namespace consumers
