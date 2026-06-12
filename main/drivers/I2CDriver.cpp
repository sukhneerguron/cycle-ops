#include "I2CDriver.hpp"
#include "esp_log.h"

namespace drivers {

static const char* TAG = "I2CDriver";

I2CDriver::~I2CDriver() {
    if (bus_handle_) {
        i2c_del_master_bus(bus_handle_);
    }
}

esp_err_t I2CDriver::init() {
    if (initialized_) return ESP_OK;

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = -1; // Auto-select
    bus_config.sda_io_num = (gpio_num_t)I2C_SDA_PIN;
    bus_config.scl_io_num = (gpio_num_t)I2C_SCL_PIN;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus initialization failed");
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized on SDA:%d SCL:%d", I2C_SDA_PIN, I2C_SCL_PIN);
    initialized_ = true;
    return ESP_OK;
}

} // namespace drivers
