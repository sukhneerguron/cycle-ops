#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "models/RideModel.hpp"
#include "events/EventBus.hpp"
#include "services/SensorFilter.hpp"

namespace services {

// ─────────────────────────────────────────────────────────────────────────────
// Default filter configurations — adjust pulses_per_rev for your setup
// ─────────────────────────────────────────────────────────────────────────────

/// Cadence sensor defaults (max 140 RPM)
static constexpr SensorFilterConfig kCadenceFilterConfig = {
    .pulses_per_rev    = 1,
    .window_size       = 4,
    .ema_alpha         = 0.3f,
    .max_rpm           = 140,
    .min_rpm           = 20,
    .zero_timeout_us   = 2'000'000,   // 2 seconds
    .debounce_fraction = 0.40f,
};

/// Wheel speed sensor defaults (max 60 km/h on 700c ≈ 475 RPM)
static constexpr SensorFilterConfig kWheelFilterConfig = {
    .pulses_per_rev    = 1,
    .window_size       = 4,
    .ema_alpha         = 0.3f,
    .max_rpm           = 475,
    .min_rpm           = 5,
    .zero_timeout_us   = 3'000'000,   // 3 seconds
    .debounce_fraction = 0.40f,
};

/// 700c wheel circumference in meters (700x25c)
static constexpr float kWheelCircumferenceMeters = 2.105f;

class CadenceService {
public:
    /// @param model      The shared RideModel where we publish our calculations (single writer).
    /// @param queue      The FreeRTOS queue where ISRs push common::SensorTimestamp events.
    /// @param event_bus  The system EventBus for publishing data-updated notifications.
    CadenceService(models::RideModel* model, QueueHandle_t queue, events::EventBus* event_bus = nullptr);
    ~CadenceService();

    void start();

private:
    static void task_entry(void* arg);
    void run();
    void process_event(uint8_t sensor_id, uint64_t timestamp_us);
    void publish_model();

    models::RideModel* model_;
    QueueHandle_t queue_;
    events::EventBus* event_bus_;
    TaskHandle_t task_handle_{nullptr};

    // --- Filter instances ---
    SensorFilter cadence_filter_{kCadenceFilterConfig};
    SensorFilter wheel_filter_{kWheelFilterConfig};

    // --- Cumulative pulse counters (raw, unfiltered) ---
    uint32_t total_crank_pulses_{0};
    uint32_t total_wheel_pulses_{0};

    /// How often we wake up to check zero-timeout when no events arrive.
    static constexpr TickType_t kPollTimeoutTicks = pdMS_TO_TICKS(500);
};

} // namespace services
