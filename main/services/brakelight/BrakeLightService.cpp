#include "BrakeLightService.hpp"
#include "esp_log.h"
#include "esp_timer.h"

namespace services {

static const char* TAG = "BrakeLightSVC";

BrakeLightService::BrakeLightService(models::RideModel* model,
                                     drivers::BrakeLightDriver* driver,
                                     events::EventBus* event_bus)
    : model_(model), driver_(driver), event_bus_(event_bus) {
}

BrakeLightService::~BrakeLightService() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
}

void BrakeLightService::start() {
    xTaskCreate(task_entry, "BrakeLightTask", 2048, this, 4, &task_handle_);
}

void BrakeLightService::task_entry(void* arg) {
    auto* self = static_cast<BrakeLightService*>(arg);
    self->run();
}

void BrakeLightService::run() {
    ESP_LOGI(TAG, "BrakeLightService task started");

    // Start in NORMAL state (25% illumination)
    apply_state(BrakeLightState::NORMAL);

    while (true) {
        // Block until RIDE_DATA_UPDATED or timeout
        EventBits_t bits = event_bus_->waitAny(
            events::EventBus::RIDE_DATA_UPDATED, kBrakeTimeoutTicks);

        if (bits & events::EventBus::RIDE_DATA_UPDATED) {
            // Clear the bit so we can detect the next update
            event_bus_->clear(events::EventBus::RIDE_DATA_UPDATED);

            // Read the latest ride data snapshot
            models::RideData data = model_->get();
            uint64_t now_us = esp_timer_get_time();

            evaluate(data.current_speed_kph, now_us);
        } else {
            // Timeout — no ride data for kBrakeTimeoutTicks.
            // Rider has fully stopped; revert to running light.
            ESP_LOGD(TAG, "Timeout — reverting to NORMAL");
            apply_state(BrakeLightState::NORMAL);
            has_prev_sample_ = false;
        }
    }
}

void BrakeLightService::evaluate(float speed_kph, uint64_t now_us) {
    if (!has_prev_sample_) {
        // First sample after start or timeout reset — just record, can't compute delta yet
        prev_speed_kph_ = speed_kph;
        prev_sample_us_ = now_us;
        has_prev_sample_ = true;
        return;
    }

    // Compute time delta in seconds
    float dt_s = static_cast<float>(now_us - prev_sample_us_) / 1'000'000.0f;

    if (dt_s < kMinDeltaSeconds) {
        // Guard against div-by-zero or stale duplicate timestamps
        return;
    }

    // Acceleration: positive = speeding up, negative = slowing down
    float accel_kph_per_sec = (speed_kph - prev_speed_kph_) / dt_s;

    ESP_LOGD(TAG, "speed=%.1f prev=%.1f dt=%.3f accel=%.2f",
             speed_kph, prev_speed_kph_, dt_s, accel_kph_per_sec);

    if (accel_kph_per_sec < -kDecelThresholdKphPerSec) {
        // Significant deceleration detected → brake!
        apply_state(BrakeLightState::BRAKING);
    } else if (speed_kph < kStandstillSpeedKph ||
               accel_kph_per_sec > kAccelThresholdKphPerSec) {
        // Rider is at standstill or actively accelerating → release brake light
        apply_state(BrakeLightState::NORMAL);
    }
    // Otherwise: coast / mild deceleration — hold current state (hysteresis)

    // Record this sample for next iteration
    prev_speed_kph_ = speed_kph;
    prev_sample_us_ = now_us;
}

void BrakeLightService::apply_state(BrakeLightState state) {
    if (state == current_state_) {
        return;
    }

    current_state_ = state;

    uint8_t duty = (state == BrakeLightState::BRAKING)
                       ? kBrakingDutyPercent
                       : kNormalDutyPercent;

    driver_->set_duty_percent(duty);

    ESP_LOGI(TAG, "State → %s (%u%% duty)",
             (state == BrakeLightState::BRAKING) ? "BRAKING" : "NORMAL", duty);
}

} // namespace services
