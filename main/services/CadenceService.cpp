#include "CadenceService.hpp"
#include "common/Types.hpp"
#include "esp_log.h"
#include "esp_timer.h"

namespace services {

static const char* TAG = "CadSVC";

CadenceService::CadenceService(models::RideModel* model, QueueHandle_t queue)
    : model_(model), queue_(queue) {
}

CadenceService::~CadenceService() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
}

void CadenceService::start() {
    xTaskCreate(task_entry, "CadenceTask", 4096, this, 5, &task_handle_);
}

void CadenceService::task_entry(void* arg) {
    auto* self = static_cast<CadenceService*>(arg);
    self->run();
}

void CadenceService::run() {
    ESP_LOGI(TAG, "CadenceService task started");
    common::SensorTimestamp event;

    while (true) {
        if (xQueueReceive(queue_, &event, kPollTimeoutTicks) == pdTRUE) {
            // Got an event from the ISR — process it
            process_event(event.sensor_id, event.timestamp_us);
        } else {
            // No event received within the poll interval. Perhaps the rider is not pedeling ?
            // Check if either sensor has timed out (rider stopped).
            uint64_t now_us = esp_timer_get_time();
            bool changed = cadence_filter_.check_timeout(now_us);
            changed |= wheel_filter_.check_timeout(now_us);
            if (changed) {
                ESP_LOGD(TAG, "Zero-timeout triggered");
                publish_model();
            }
        }
    }
}

void CadenceService::process_event(uint8_t sensor_id, uint64_t timestamp_us) {
    if (sensor_id == common::SENSOR_ID_CADENCE) {
        if (cadence_filter_.feed(timestamp_us)) {
            total_crank_pulses_++;
            ESP_LOGI(TAG, "CPulse at %llu, pulses %lu, cadence %u RPM",
                     (unsigned long long)timestamp_us,
                     (unsigned long)total_crank_pulses_,
                     cadence_filter_.current_rpm());
            publish_model();
        }
    } else if (sensor_id == common::SENSOR_ID_WHEEL) {
        if (wheel_filter_.feed(timestamp_us)) {
            total_wheel_pulses_++;
            ESP_LOGI(TAG, "Wpulse at %llu, pulses %lu, wheel RPM: %u",
                     (unsigned long long)timestamp_us,
                     (unsigned long)total_wheel_pulses_,
                     wheel_filter_.current_rpm());
            publish_model();
        }
    }
}

void CadenceService::publish_model() {
    models::RideData data = model_->get();

    // Smoothed RPM values from the filter pipeline
    data.current_cadence = cadence_filter_.current_rpm();
    data.current_wheel_rpm = wheel_filter_.current_rpm();

    // Cumulative revolutions — convert pulses to full revolutions
    // Integer division is correct: a partial rev doesn't count (per BLE CSC spec)
    data.total_crank_revolutions = total_crank_pulses_ / kCadenceFilterConfig.pulses_per_rev;
    data.total_wheel_revolutions = total_wheel_pulses_ / kWheelFilterConfig.pulses_per_rev;

    // BLE CSC event time fields: 1/1024 second resolution
    // Use the last accepted event timestamp from each filter
    uint64_t crank_event_us = cadence_filter_.last_event_time_us();
    uint64_t wheel_event_us = wheel_filter_.last_event_time_us();

    if (crank_event_us > 0) {
        data.last_crank_event_time = static_cast<uint16_t>(
            (crank_event_us * 1024ULL) / 1'000'000ULL);
    }
    if (wheel_event_us > 0) {
        data.last_wheel_event_time = static_cast<uint16_t>(
            (wheel_event_us * 1024ULL) / 1'000'000ULL);
    }

    model_->update(data);
}

} // namespace services
