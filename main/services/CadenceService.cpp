#include "CadenceService.hpp"
#include "common/Types.hpp"
#include "esp_log.h"

namespace services {

static const char* TAG = "CadenceService";

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
        if (xQueueReceive(queue_, &event, portMAX_DELAY) == pdTRUE) {
            process_event(event.sensor_id, event.timestamp_us);
        }
    }
}

void CadenceService::process_event(uint8_t sensor_id, uint64_t timestamp_us) {
    bool updated = false;

    if (sensor_id == common::SENSOR_ID_CADENCE) {
        if (timestamp_us - last_cadence_time_us_ > DEBOUNCE_TIME_US) {
            if (last_cadence_time_us_ > 0) {
                uint64_t delta_us = timestamp_us - last_cadence_time_us_;
                // Cadence in RPM: (1,000,000 us / delta_us) * 60
                current_cadence_ = static_cast<uint16_t>((60000000ULL) / delta_us);
            }
            total_crank_revolutions_++;
            ESP_LOGI(TAG, "Crank rev registered, total: %lu", (unsigned long)total_crank_revolutions_);
            last_cadence_time_us_ = timestamp_us;
            updated = true;
        }
    } else if (sensor_id == common::SENSOR_ID_RPM) {
        if (timestamp_us - last_rpm_time_us_ > DEBOUNCE_TIME_US) {
            if (last_rpm_time_us_ > 0) {
                uint64_t delta_us = timestamp_us - last_rpm_time_us_;
                current_rpm_ = static_cast<uint16_t>((60000000ULL) / delta_us);
            }
            total_wheel_revolutions_++;
            ESP_LOGI(TAG, "Wheel rev registered, total: %lu", (unsigned long)total_wheel_revolutions_);
            last_rpm_time_us_ = timestamp_us;
            updated = true;
        }
    }

    if (updated) {
        models::RideData data = model_->get();
        data.current_cadence = current_cadence_;
        data.total_crank_revolutions = total_crank_revolutions_;
        data.current_rpm = current_rpm_;
        data.total_wheel_revolutions = total_wheel_revolutions_;
        
        // BLE CSC time fields are in 1/1024s. We convert from microseconds.
        if (sensor_id == common::SENSOR_ID_CADENCE) {
            data.last_crank_event_time = static_cast<uint16_t>((timestamp_us * 1024) / 1000000);
        } else {
            data.last_wheel_event_time = static_cast<uint16_t>((timestamp_us * 1024) / 1000000);
        }

        model_->update(data);
    }
}

} // namespace services
