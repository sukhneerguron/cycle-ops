#include "BrakeLightDriver.hpp"
#include "driver/ledc.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace drivers {

static const char* TAG = "BrakeLightDrv";

// LEDC configuration — Timer 1 / Channel 1 to avoid conflicts with display backlight.
static constexpr ledc_timer_t   kLedcTimer   = LEDC_TIMER_1;
static constexpr ledc_channel_t kLedcChannel = LEDC_CHANNEL_1;
static constexpr ledc_mode_t    kLedcMode    = LEDC_LOW_SPEED_MODE;
static constexpr int            kFrequencyHz = 5000;   // 5 kHz — well above flicker perception
static constexpr ledc_timer_bit_t kDutyResolution = LEDC_TIMER_13_BIT;  // 0–8191
static constexpr uint32_t       kMaxDuty     = (1 << 13) - 1;           // 8191

void BrakeLightDriver::init() {
    if (initialized_) return;

    // Configure LEDC timer
    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode      = kLedcMode;
    timer_config.timer_num       = kLedcTimer;
    timer_config.duty_resolution = kDutyResolution;
    timer_config.freq_hz         = kFrequencyHz;
    timer_config.clk_cfg         = LEDC_AUTO_CLK;

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(err));
        return;
    }

    // Configure LEDC channel on the brake light pin
    ledc_channel_config_t channel_config = {};
    channel_config.speed_mode = kLedcMode;
    channel_config.channel    = kLedcChannel;
    channel_config.timer_sel  = kLedcTimer;
    channel_config.intr_type  = LEDC_INTR_DISABLE;
    channel_config.gpio_num   = CONFIG_BRAKE_LIGHT_PIN;
    channel_config.duty       = 0;
    channel_config.hpoint     = 0;

    err = ledc_channel_config(&channel_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(err));
        return;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Initialized on GPIO %d (Timer %d, Channel %d, %d Hz)",
             CONFIG_BRAKE_LIGHT_PIN, kLedcTimer, kLedcChannel, kFrequencyHz);
}

void BrakeLightDriver::set_duty_percent(uint8_t percent) {
    if (!initialized_) return;

    if (percent > 100) {
        percent = 100;
    }

    uint32_t duty = (static_cast<uint32_t>(percent) * kMaxDuty) / 100;
    ledc_set_duty(kLedcMode, kLedcChannel, duty);
    ledc_update_duty(kLedcMode, kLedcChannel);
}

} // namespace drivers
