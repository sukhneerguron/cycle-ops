#include "TouchDriver.hpp"
#include "esp_lcd_touch_cst328.h"
#include "esp_log.h"
#include "sdkconfig.h"

namespace drivers {

static const char* TAG = "TouchDriver";

TouchDriver* TouchDriver::s_instance = nullptr;

TouchDriver::~TouchDriver() {
    if (touch_handle_) {
        esp_lcd_touch_del(touch_handle_);
    }
    if (io_handle_) {
        esp_lcd_panel_io_del(io_handle_);
    }
    if (indev_) {
        lv_indev_delete(indev_);
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void TouchDriver::init(i2c_master_bus_handle_t i2c_bus, events::EventBus& event_bus) {
    if (initialized_) return;

    ESP_LOGI(TAG, "Initializing CST328 Touch Panel");
    
    event_bus_ = &event_bus;
    s_instance = this;

    // Manually initialize all fields to avoid C++ -Wmissing-field-initializers
    // from the C macro ESP_LCD_TOUCH_IO_I2C_CST328_CONFIG()
    esp_lcd_panel_io_i2c_config_t tp_io_config = {};
    tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_CST328_ADDRESS;
    tp_io_config.control_phase_bytes = 1;
    tp_io_config.dc_bit_offset = 0;
    tp_io_config.lcd_cmd_bits = 16;
    tp_io_config.scl_speed_hz = 400000;
    tp_io_config.flags.disable_control_phase = 1;
    
    esp_err_t err = esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &io_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO for touch");
        return;
    }

    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = CONFIG_DISPLAY_WIDTH;
    tp_cfg.y_max = CONFIG_DISPLAY_HEIGHT;
    tp_cfg.rst_gpio_num = (gpio_num_t)CONFIG_TOUCH_RST;
    tp_cfg.int_gpio_num = (gpio_num_t)CONFIG_TOUCH_INT;
    tp_cfg.levels.reset = 0;
    tp_cfg.levels.interrupt = 0;
    tp_cfg.flags.swap_xy = 0;
    tp_cfg.flags.mirror_x = 0;
    tp_cfg.flags.mirror_y = 0;
    tp_cfg.interrupt_callback = touch_interrupt_cb;

    err = esp_lcd_touch_new_i2c_cst328(io_handle_, &tp_cfg, &touch_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create CST328 touch handle");
        return;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "CST328 initialized successfully");
}

void TouchDriver::register_lvgl_indev(lv_display_t* disp) {
    if (!initialized_ || !disp) return;

    ESP_LOGI(TAG, "Registering LVGL input device");
    
    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_, lv_touch_read_cb);
    lv_indev_set_user_data(indev_, this);
    lv_indev_set_display(indev_, disp);
}

void TouchDriver::touch_interrupt_cb(esp_lcd_touch_handle_t tp) {
    if (s_instance && s_instance->event_bus_) {
        // Wake up DisplayTask to call lv_timer_handler
        s_instance->event_bus_->publishFromISR(events::EventBus::TOUCH_INTERRUPT);
    }
}

void TouchDriver::lv_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* self = static_cast<TouchDriver*>(lv_indev_get_user_data(indev));
    if (!self || !self->touch_handle_) return;

    esp_lcd_touch_point_data_t point_data[1] = {};
    uint8_t touch_cnt = 0;

    // Read the touch data from the controller
    esp_lcd_touch_read_data(self->touch_handle_);
    esp_err_t err = esp_lcd_touch_get_data(self->touch_handle_, point_data, &touch_cnt, 1);

    if (err == ESP_OK && touch_cnt > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = point_data[0].x;
        data->point.y = point_data[0].y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

} // namespace drivers
