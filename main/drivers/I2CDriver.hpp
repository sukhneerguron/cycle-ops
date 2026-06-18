#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

namespace drivers {

class I2CDriver {
public:
    I2CDriver() = default;
    ~I2CDriver();

    esp_err_t init();
    
    // Provide the bus handle so other drivers can attach to it
    i2c_master_bus_handle_t get_bus_handle() const { return bus_handle_; }

private:

    static constexpr uint32_t I2C_CLK_SPEED = 400000;
    
    i2c_master_bus_handle_t bus_handle_{nullptr};
    bool initialized_{false};
};

} // namespace drivers
