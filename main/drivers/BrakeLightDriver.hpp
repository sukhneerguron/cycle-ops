#pragma once
#include <cstdint>

namespace drivers {

/// Driver that owns the LEDC PWM peripheral for the brake light GPIO.
/// The brake light is driven via a MOSFET — duty cycle controls brightness.
class BrakeLightDriver {
public:
    /// Initialize LEDC Timer 1 / Channel 1 on CONFIG_BRAKE_LIGHT_PIN.
    void init();

    /// Set duty cycle: 0–100 (percent).
    /// 0 = fully off, 100 = fully on.
    void set_duty_percent(uint8_t percent);

private:
    bool initialized_{false};
};

} // namespace drivers
