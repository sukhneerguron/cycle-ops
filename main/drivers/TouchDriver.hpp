#pragma once

#include "driver/i2c_master.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"
#include "events/EventBus.hpp"

namespace drivers {

class TouchDriver {
public:
    TouchDriver() = default;
    ~TouchDriver();

    void init(i2c_master_bus_handle_t i2c_bus, events::EventBus& event_bus);
    
    void register_lvgl_indev(lv_display_t* disp);
    lv_indev_t* get_indev() const { return indev_; }

private:
    static void touch_interrupt_cb(esp_lcd_touch_handle_t tp);
    static void lv_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data);

    esp_lcd_panel_io_handle_t io_handle_{nullptr};
    esp_lcd_touch_handle_t touch_handle_{nullptr};
    lv_indev_t* indev_{nullptr};
    
    events::EventBus* event_bus_{nullptr};
    
    bool initialized_{false};

    // Keep a static instance pointer for the ISR callback since esp_lcd_touch_config_t 
    // interrupt_callback does not support user context in older IDF versions
    static TouchDriver* s_instance;
};

} // namespace drivers
