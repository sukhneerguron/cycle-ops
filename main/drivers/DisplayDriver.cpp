#include "DisplayDriver.hpp"
#include "esp_lcd_io_i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"

namespace drivers {

static const char *TAG = "DisplayDriver";

DisplayDriver::DisplayDriver() { mutex_ = xSemaphoreCreateRecursiveMutex(); }

DisplayDriver::~DisplayDriver() {
  if (mutex_) {
    vSemaphoreDelete(mutex_);
  }
}

void DisplayDriver::init(i2c_master_bus_handle_t i2c_bus) {
  if (initialized_)
    return;

  ESP_LOGI(TAG, "Initializing SSD1306 Panel");

  esp_lcd_panel_io_i2c_config_t io_config = {};
  io_config.dev_addr = 0x3C; // Default I2C address for SSD1306
  io_config.control_phase_bytes = 1;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  io_config.dc_bit_offset = 6;
  io_config.scl_speed_hz = 400000;

  // Use the provided I2C bus handle with the correct v6.0 API
  esp_err_t err = esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO");
    return;
  }

  esp_lcd_panel_dev_config_t panel_config = {};
  panel_config.reset_gpio_num = GPIO_NUM_NC; // No reset pin
  panel_config.bits_per_pixel = 1;

  err = esp_lcd_new_panel_ssd1306(io_handle_, &panel_config, &panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create SSD1306 panel");
    return;
  }

  err = esp_lcd_panel_reset(panel_handle_);
  err = esp_lcd_panel_init(panel_handle_);
  err = esp_lcd_panel_disp_on_off(panel_handle_, true);
  err = esp_lcd_panel_mirror(panel_handle_, true, true); // Flip 180 degrees

  ESP_LOGI(TAG, "Initializing LVGL");
  lv_init();

  lv_display_ = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_user_data(lv_display_, panel_handle_);
  lv_display_set_flush_cb(lv_display_, flush_callback);
  lv_display_set_color_format(lv_display_, LV_COLOR_FORMAT_RGB565);

  // Allocate buffer for 16-bit RGB565 display full-screen rendering
  // Buffer size = width * height * 2 bytes (16-bit)
  size_t buf_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2;
  void *buf1 =
      heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  void *buf2 =
      heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  lv_display_set_buffers(lv_display_, buf1, buf2, buf_size,
                         LV_DISPLAY_RENDER_MODE_FULL);

  // Set up tick timer
  const esp_timer_create_args_t tick_timer_args = {
      .callback = &lv_tick_task,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "lvgl_tick",
      .skip_unhandled_events = false};
  esp_timer_handle_t tick_timer;
  esp_timer_create(&tick_timer_args, &tick_timer);
  esp_timer_start_periodic(tick_timer, 2 * 1000); // 2 ms

  // NOTE: The LVGL event loop (lv_timer_handler) is owned by ui::DisplayTask,
  //       not the driver. Do not create the task here.

  initialized_ = true;
}

void DisplayDriver::lv_tick_task(void *arg) { lv_tick_inc(2); }

void DisplayDriver::flush_callback(lv_display_t *disp, const lv_area_t *area,
                                   uint8_t *px_map) {
  esp_lcd_panel_handle_t panel_handle =
      (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  int width = area->x2 - area->x1 + 1;
  int height = area->y2 - area->y1 + 1;

  // Ensure we are processing a full screen flush
  if (width != DISPLAY_WIDTH || height != DISPLAY_HEIGHT) {
    lv_display_flush_ready(disp);
    return;
  }

  // Allocate a temporary buffer for the 1-bit SSD1306 format (1024 bytes)
  static uint8_t ssd1306_buf[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];
  memset(ssd1306_buf, 0, sizeof(ssd1306_buf));

  uint16_t *rgb_buf = (uint16_t *)px_map;

  // Convert RGB565 horizontal layout to SSD1306 vertical page layout
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      uint16_t color = rgb_buf[y * DISPLAY_WIDTH + x];
      // If color is not completely black (0x0000), set the bit in the
      // monochrome buffer
      if (color != 0x0000) {
        // byte index = x + (y / 8) * DISPLAY_WIDTH
        // bit index = y % 8
        ssd1306_buf[x + (y / 8) * DISPLAY_WIDTH] |= (1 << (y % 8));
      }
    }
  }

  esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                            ssd1306_buf);
  lv_display_flush_ready(disp);
}

} // namespace drivers
