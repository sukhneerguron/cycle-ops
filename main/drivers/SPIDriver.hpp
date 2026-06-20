#pragma once

#include "driver/spi_master.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

namespace drivers {

class SPIDriver {
public:
    SPIDriver() = default;
    ~SPIDriver();

    esp_err_t init();
    spi_host_device_t get_host() const { return host_; }

private:
    spi_host_device_t host_{SPI2_HOST};
    bool initialized_{false};
};

} // namespace drivers
