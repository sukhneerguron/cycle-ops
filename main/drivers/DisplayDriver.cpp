#include "DisplayDriver.hpp"
#include "driver/gpio.h"
#include "esp_lcd_io_spi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "misc/lv_color.h"
#include "sdkconfig.h"

namespace drivers {

static const char *TAG = "DisplayDriver";

DisplayDriver::DisplayDriver() { mutex_ = xSemaphoreCreateRecursiveMutex(); }

DisplayDriver::~DisplayDriver() {
  if (mutex_) {
    vSemaphoreDelete(mutex_);
  }
}

void DisplayDriver::init(spi_host_device_t spi_host) {
  if (initialized_)
    return;

  ESP_LOGI(TAG, "Initializing ST7789 Panel via SPI");

  // Backlight config
  gpio_config_t bk_gpio_config = {.pin_bit_mask = 1ULL << CONFIG_DISPLAY_BL,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&bk_gpio_config);
  gpio_set_level((gpio_num_t)CONFIG_DISPLAY_BL, 1); // Turn on backlight

  esp_lcd_panel_io_spi_config_t io_config = {};
  io_config.dc_gpio_num = (gpio_num_t)CONFIG_DISPLAY_DC;
  io_config.cs_gpio_num = (gpio_num_t)CONFIG_DISPLAY_CS;
  io_config.pclk_hz = 40 * 1000 * 1000; // 40MHz
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  io_config.spi_mode = 0;
  io_config.trans_queue_depth = 10;
  io_config.on_color_trans_done = on_color_trans_done;
  io_config.user_ctx = this;

  esp_err_t err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)spi_host,
                                           &io_config, &io_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO");
    return;
  }

  esp_lcd_panel_dev_config_t panel_config = {};
  panel_config.reset_gpio_num = (gpio_num_t)CONFIG_DISPLAY_RST;
  panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
  panel_config.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE;
  panel_config.bits_per_pixel = 16;

  err = esp_lcd_new_panel_st7789(io_handle_, &panel_config, &panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create ST7789 panel");
    return;
  }

  err = esp_lcd_panel_reset(panel_handle_);
  err = esp_lcd_panel_init(panel_handle_);
  // Invert colors if necessary (ST7789 typically requires it)
  err = esp_lcd_panel_invert_color(panel_handle_, true);
  // Set orientation to Portrait
  err = esp_lcd_panel_swap_xy(panel_handle_, false);
  err = esp_lcd_panel_mirror(panel_handle_, false, false);
  err = esp_lcd_panel_disp_on_off(panel_handle_, true);

  ESP_LOGI(TAG, "Initializing LVGL");
  lv_init();

  lv_display_ = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_display_set_user_data(lv_display_, panel_handle_);
  lv_display_set_flush_cb(lv_display_, flush_callback);
  lv_display_set_color_format(lv_display_, LV_COLOR_FORMAT_RGB565);

  // Allocate buffer for 16-bit RGB565 display rendering
  // We use 1/10th of the screen to save SRAM (requires MALLOC_CAP_DMA for SPI
  // DMA)
  size_t buf_size = DISPLAY_WIDTH * (DISPLAY_HEIGHT / 10) * 2;
  void *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  void *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lv_display_set_buffers(lv_display_, buf1, buf2, buf_size,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

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

bool DisplayDriver::on_color_trans_done(esp_lcd_panel_io_handle_t panel_io,
                                        esp_lcd_panel_io_event_data_t *edata,
                                        void *user_ctx) {
  DisplayDriver *driver = static_cast<DisplayDriver *>(user_ctx);
  lv_display_t *disp = driver->get_lv_display();
  if (disp) {
    lv_display_flush_ready(disp);
  }
  return false;
}

void DisplayDriver::flush_callback(lv_display_t *disp, const lv_area_t *area,
                                   uint8_t *px_map) {
  esp_lcd_panel_handle_t panel_handle =
      (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  // Directly push the RGB565 pixel map via SPI DMA
  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1,
                            area->y2 + 1, px_map);
}

} // namespace drivers
