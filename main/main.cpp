#include "App.hpp"
#include "esp_log.h"
#include "nvs_flash.h"

extern "C" void app_main() {
    // Initialize NVS (required for BLE bonding and internal configuration)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_log_level_set("NimBLE", ESP_LOG_WARN);
    ESP_LOGI("MAIN", "Starting Application...");

    // Create and start the application orchestrator
    // We use dynamic allocation or a static instance to keep it alive
    static application::App app;
    app.start();
}
