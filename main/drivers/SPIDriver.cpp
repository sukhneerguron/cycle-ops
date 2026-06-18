#include "SPIDriver.hpp"
#include "esp_log.h"
#include "sdkconfig.h"

namespace drivers {

static const char* TAG = "SPIDriver";

SPIDriver::~SPIDriver() {
    if (initialized_) {
        spi_bus_free(host_);
    }
}

esp_err_t SPIDriver::init() {
    if (initialized_) return ESP_OK;

    ESP_LOGI(TAG, "Initializing SPI bus on MOSI:%d, SCLK:%d", CONFIG_SPI_MOSI, CONFIG_SPI_SCLK);

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = CONFIG_SPI_MOSI;
    buscfg.miso_io_num = -1; // Not used for display, can be updated later for SD card
    buscfg.sclk_io_num = CONFIG_SPI_SCLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 320 * 240 * 2 + 8; // Max size for full screen update

    esp_err_t ret = spi_bus_initialize(host_, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return ret;
    }

    initialized_ = true;
    return ESP_OK;
}

} // namespace drivers
