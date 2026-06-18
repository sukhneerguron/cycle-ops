#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "driver/spi_master.h"

namespace drivers {

class DisplayDriver {
public:
    DisplayDriver();
    ~DisplayDriver();

    void init(spi_host_device_t spi_host);
    
    void lock() {
        if (mutex_) xSemaphoreTakeRecursive(mutex_, portMAX_DELAY);
    }

    void unlock() {
        if (mutex_) xSemaphoreGiveRecursive(mutex_);
    }

private:
    static void lv_tick_task(void *arg);
    static void flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

    esp_lcd_panel_io_handle_t io_handle_{nullptr};
    esp_lcd_panel_handle_t panel_handle_{nullptr};
    lv_display_t *lv_display_{nullptr};
    
    SemaphoreHandle_t mutex_{nullptr};
    
    static constexpr int DISPLAY_WIDTH = 240;
    static constexpr int DISPLAY_HEIGHT = 320;
    
    bool initialized_{false};
};

} // namespace drivers
