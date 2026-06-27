#include "App.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void stats_task(void *arg) {
    char *stats_buf = (char *)malloc(1024);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (stats_buf) {
            vTaskGetRunTimeStats(stats_buf);
            ESP_LOGI("STATS", "Run Time Stats:\n%s", stats_buf);
        }
    }
}

// ESP-IDF requires this function to return bool when CONFIG_FREERTOS_USE_IDLE_HOOK is enabled
extern "C" __attribute__((weak)) bool vApplicationIdleHook(void) {
    return true; 
}

extern "C" uint32_t my_lvgl_idle_percent(void) {
    static uint32_t last_total_run_time = 0;
    static uint32_t last_idle_run_time = 0;

    UBaseType_t array_size = uxTaskGetNumberOfTasks();
    TaskStatus_t *status_array = (TaskStatus_t *)malloc(array_size * sizeof(TaskStatus_t));
    if (!status_array) return 0;

    uint32_t total_run_time = 0;
    array_size = uxTaskGetSystemState(status_array, array_size, &total_run_time);

    uint32_t idle_run_time = 0;
    for (UBaseType_t i = 0; i < array_size; i++) {
        // ESP-IDF SMP FreeRTOS names idle tasks "IDLE" or "IDLE0", "IDLE1"
        if (strncmp(status_array[i].pcTaskName, "IDLE", 4) == 0) {
            idle_run_time += status_array[i].ulRunTimeCounter;
        }
    }
    free(status_array);

    if (total_run_time <= last_total_run_time) return 0;

    uint32_t total_delta = total_run_time - last_total_run_time;
    uint32_t idle_delta = idle_run_time - last_idle_run_time;

    last_total_run_time = total_run_time;
    last_idle_run_time = idle_run_time;

    // We have 2 cores, so max idle delta is 2 * total_delta. 
    // We average the idle percentage across the two cores.
    uint32_t idle_percent = (idle_delta * 100) / (total_delta * 2);
    if (idle_percent > 100) idle_percent = 100;

    return idle_percent;
}

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

    // Start a debug task to print CPU usage stats every 5 seconds
    //xTaskCreate(stats_task, "stats_task", 4096, NULL, 1, NULL);
}
