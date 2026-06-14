#pragma once

#include <cstdint>

namespace services {

/// Configuration for a single sensor's filter pipeline.
/// All tuning parameters are grouped here for clarity.
struct SensorFilterConfig {
  uint8_t pulses_per_rev; ///< Number of magnets on the disc (1, 2, etc.)
  uint8_t window_size;    ///< Sliding window depth for averaging (max 8)
  float ema_alpha; ///< EMA smoothing factor (0.0 = frozen, 1.0 = no smoothing)
  uint16_t
      max_rpm; ///< Physical RPM upper clamp (after pulses_per_rev correction)
  uint16_t min_rpm; ///< Below this → treat as stopped / noise
  uint64_t
      zero_timeout_us;     ///< Snap to 0 after this much silence (microseconds)
  float debounce_fraction; ///< Fraction of min expected period used as debounce
};

/// Portable sensor filter implementing:
///   ① Adaptive Debounce
///   ② Sliding window (ring buffer of inter-event deltas → mean)
///   ③ EMA smoothing (on RPM value)
///   ④ Pulses-per-rev correction
///   ⑤ Clamping to [min_rpm, max_rpm]
///   ⑥ Zero-timeout detection
///
/// This class has NO FreeRTOS or ESP-IDF dependencies.
/// It can be compiled and unit-tested on a desktop PC.
class SensorFilter {
public:
  explicit SensorFilter(const SensorFilterConfig &cfg);

  /// Feed a new event timestamp (microseconds, monotonic).
  /// Returns true if the event was accepted (not debounced).
  /// After acceptance, current_rpm() reflects the new smoothed value.
  bool feed(uint64_t timestamp_us);

  /// Call periodically with the current time (microseconds).
  /// If the time since the last accepted event exceeds zero_timeout_us,
  /// RPM is snapped to 0. Returns true if a zero-transition occurred.
  bool check_timeout(uint64_t now_us);

  /// The current smoothed, corrected, clamped RPM output.
  uint16_t current_rpm() const { return output_rpm_; }

  /// The timestamp of the last accepted (non-debounced) event.
  uint64_t last_event_time_us() const { return last_event_us_; }

  /// Reset all internal state (e.g. on ride start).
  void reset();

private:
  /// Push a new delta into the ring buffer.
  void push_delta(uint64_t delta_us);

  /// Compute the arithmetic mean of all deltas in the ring buffer.
  uint64_t mean_delta_us() const;

  /// The full pipeline: mean delta → raw RPM → EMA → pulses_per_rev → clamp.
  void update_output();

  SensorFilterConfig config_;
  uint64_t debounce_threshold_us_; ///< Derived from config at construction

  // --- Ring buffer for sliding window ---
  static constexpr uint8_t kMaxWindowSize = 8;
  uint64_t deltas_[kMaxWindowSize]{};
  uint8_t head_{0};
  uint8_t count_{0};

  // --- Filter state ---
  uint64_t last_event_us_{0};
  bool has_first_event_{false};
  float ema_rpm_{0.0f};
  bool ema_primed_{false}; ///< False until first EMA sample
  uint16_t output_rpm_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Inline implementation (header-only for ease of use in embedded context)
// ─────────────────────────────────────────────────────────────────────────────

inline SensorFilter::SensorFilter(const SensorFilterConfig &cfg)
    : config_(cfg) {
  // Derive debounce threshold from max expected pulse frequency:
  //   min_period_us = 60,000,000 / (max_rpm × pulses_per_rev)
  //   debounce = min_period × debounce_fraction
  if (config_.max_rpm > 0 && config_.pulses_per_rev > 0) {
    uint64_t min_period_us =
        60'000'000ULL /
        (static_cast<uint64_t>(config_.max_rpm) * config_.pulses_per_rev);
    debounce_threshold_us_ = static_cast<uint64_t>(
        static_cast<float>(min_period_us) * config_.debounce_fraction);
  } else {
    debounce_threshold_us_ = 0;
  }

  // Clamp window_size to valid range
  if (config_.window_size > kMaxWindowSize) {
    config_.window_size = kMaxWindowSize;
  }
  if (config_.window_size == 0) {
    config_.window_size = 1;
  }
}

inline void SensorFilter::reset() {
  head_ = 0;
  count_ = 0;
  last_event_us_ = 0;
  has_first_event_ = false;
  ema_rpm_ = 0.0f;
  ema_primed_ = false;
  output_rpm_ = 0;
  for (uint8_t i = 0; i < kMaxWindowSize; ++i) {
    deltas_[i] = 0;
  }
}

inline bool SensorFilter::feed(uint64_t timestamp_us) {
  // ① First event — just record, no delta to compute yet
  if (!has_first_event_) {
    last_event_us_ = timestamp_us;
    has_first_event_ = true;
    return true;
  }

  // Guard against timestamps going backwards (should not happen with
  // monotonic esp_timer_get_time, but defensive)
  if (timestamp_us <= last_event_us_) {
    return false;
  }

  uint64_t delta_us = timestamp_us - last_event_us_;

  // If the wheel stopped (timeout exceeded), do not compute a delta!
  // Treat this as the very first pulse of a new ride to prevent massive
  // deltas from destroying the adaptive debounce calculations.
  if (delta_us > config_.zero_timeout_us) {
    reset();
    last_event_us_ = timestamp_us;
    has_first_event_ = true;
    return true;
  }

  // ① Adaptive Debounce
  // Base debounce is calculated from max_rpm. If we are moving slower than
  // max_rpm, we dynamically expand the debounce window based on the current
  // running average. wheel: A standard 700c wheel has a circumference of ~2.1
  // meters. At 10 km/h, one revolution takes about 0.75 seconds. This adaptive
  // debounce window is 40% of that: 0.30 seconds. thats 3G acceleration

  uint64_t active_debounce = debounce_threshold_us_;
  uint64_t current_mean = mean_delta_us();
  if (current_mean > 0) {
    uint64_t dynamic_debounce = static_cast<uint64_t>(
        static_cast<float>(current_mean) * config_.debounce_fraction);
    if (dynamic_debounce > active_debounce) {
      active_debounce = dynamic_debounce;
    }
  } else {
    // Startup phase (count_ == 0).
    // It is physically impossible to instantly accelerate to max_rpm from 0.
    // We apply a 5x multiplier to the base debounce for the very first pulse
    // to aggressively reject slow analog bounces and prevent EMA poisoning.
    active_debounce = debounce_threshold_us_ * 5;
  }

  // Reject if too close to previous event
  if (delta_us < active_debounce) {
    return false;
  }

  // ② Record this accepted event
  last_event_us_ = timestamp_us;

  // ③ Push delta into sliding window
  push_delta(delta_us);

  // ④–⑥ Run the full output pipeline
  update_output();

  return true;
}

inline bool SensorFilter::check_timeout(uint64_t now_us) {
  // Nothing to time out if we never received an event
  if (!has_first_event_) {
    return false;
  }

  // Already at zero — no transition
  if (output_rpm_ == 0) {
    return false;
  }

  if (now_us > last_event_us_ &&
      (now_us - last_event_us_) > config_.zero_timeout_us) {
    // Snap to zero
    output_rpm_ = 0;
    ema_rpm_ = 0.0f;
    ema_primed_ = false;
    count_ = 0;
    head_ = 0;
    has_first_event_ = false;
    return true;
  }

  return false;
}

inline void SensorFilter::push_delta(uint64_t delta_us) {
  deltas_[head_] = delta_us;
  head_ = (head_ + 1) % config_.window_size;
  if (count_ < config_.window_size) {
    count_++;
  }
}

inline uint64_t SensorFilter::mean_delta_us() const {
  if (count_ == 0)
    return 0;

  uint64_t sum = 0;
  for (uint8_t i = 0; i < count_; ++i) {
    sum += deltas_[i];
  }
  return sum / count_;
}

inline void SensorFilter::update_output() {
  uint64_t avg_delta = mean_delta_us();
  if (avg_delta == 0) {
    output_rpm_ = 0;
    return;
  }

  // Raw pulse RPM from mean delta
  float raw_pulse_rpm = 60'000'000.0f / static_cast<float>(avg_delta);

  // ③ EMA smoothing
  if (!ema_primed_) {
    // First sample — seed the EMA
    ema_rpm_ = raw_pulse_rpm;
    ema_primed_ = true;
  } else {
    ema_rpm_ = config_.ema_alpha * raw_pulse_rpm +
               (1.0f - config_.ema_alpha) * ema_rpm_;
  }

  // ④ Pulses-per-rev correction: convert pulse RPM to physical RPM
  float physical_rpm = ema_rpm_ / static_cast<float>(config_.pulses_per_rev);

  // ⑤ Clamping
  if (physical_rpm < static_cast<float>(config_.min_rpm)) {
    output_rpm_ = 0; // Below minimum → treat as stopped
    return;
  }
  if (physical_rpm > static_cast<float>(config_.max_rpm)) {
    physical_rpm = static_cast<float>(config_.max_rpm);
  }

  output_rpm_ = static_cast<uint16_t>(physical_rpm + 0.5f); // Round
}

} // namespace services
