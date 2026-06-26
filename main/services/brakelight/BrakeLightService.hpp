#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "models/RideModel.hpp"
#include "drivers/BrakeLightDriver.hpp"
#include "events/EventBus.hpp"

namespace services {

/// Brake light illumination state.
enum class BrakeLightState : uint8_t {
    NORMAL,   ///< 25% duty — tail light / running light
    BRAKING,  ///< 100% duty — active braking detected
};

/// Service that monitors RideModel speed to detect deceleration and controls
/// the brake light driver accordingly.
///
/// Architecture:
///   - Reader of RideModel (writer: CadenceService)
///   - Wakes on RIDE_DATA_UPDATED events from EventBus
///   - Commands BrakeLightDriver with duty cycle changes
///
/// The deceleration algorithm computes rate-of-change of speed between
/// consecutive RIDE_DATA_UPDATED samples. A timeout mechanism reverts
/// to NORMAL if no updates arrive (rider fully stopped).
class BrakeLightService {
public:
    /// @param model   The shared RideModel to read speed from (single writer: CadenceService).
    /// @param driver  The BrakeLightDriver that owns the LEDC PWM peripheral.
    /// @param event_bus The system EventBus for receiving RIDE_DATA_UPDATED notifications.
    BrakeLightService(models::RideModel* model,
                      drivers::BrakeLightDriver* driver,
                      events::EventBus* event_bus);
    ~BrakeLightService();

    void start();

private:
    static void task_entry(void* arg);
    void run();

    /// Evaluate the current speed sample and transition state if thresholds are crossed.
    void evaluate(float speed_kph, uint64_t now_us);

    /// Apply a new brake light state, commanding the driver if it changed.
    void apply_state(BrakeLightState state);

    models::RideModel* model_;
    drivers::BrakeLightDriver* driver_;
    events::EventBus* event_bus_;
    TaskHandle_t task_handle_{nullptr};

    BrakeLightState current_state_{BrakeLightState::NORMAL};

    // --- Speed history for acceleration computation ---
    float prev_speed_kph_{0.0f};
    uint64_t prev_sample_us_{0};
    bool has_prev_sample_{false};

    // --- Tuning constants ---

    /// Deceleration magnitude (km/h per second) to trigger BRAKING state.
    /// ~0.08g — comfortable braking on flat ground typically produces 3–6 km/h/s.
    static constexpr float kDecelThresholdKphPerSec = 3.0f;

    /// Acceleration magnitude (km/h per second) to release back to NORMAL.
    /// Asymmetric hysteresis prevents rapid toggling near zero deceleration.
    static constexpr float kAccelThresholdKphPerSec = 1.5f;

    /// Speed below which the rider is considered at a standstill (km/h).
    static constexpr float kStandstillSpeedKph = 0.5f;

    /// If no RIDE_DATA_UPDATED arrives within this period, revert to NORMAL.
    /// Matches the wheel filter zero_timeout_us of 3 seconds.
    static constexpr TickType_t kBrakeTimeoutTicks = pdMS_TO_TICKS(3000);

    /// Minimum time delta between samples to avoid div-by-zero / stale duplicates.
    static constexpr float kMinDeltaSeconds = 0.01f;

    /// Duty cycle for NORMAL (running light) mode.
    static constexpr uint8_t kNormalDutyPercent  = 25;

    /// Duty cycle for BRAKING mode.
    static constexpr uint8_t kBrakingDutyPercent = 100;
};

} // namespace services
