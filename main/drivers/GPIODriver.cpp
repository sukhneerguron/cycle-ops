#include "GPIODriver.hpp"
#include "common/Types.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

namespace drivers {

static const char* TAG = "GPIODriver";

static constexpr gpio_num_t PIN_CADENCE = GPIO_NUM_9;
static constexpr gpio_num_t PIN_RPM = GPIO_NUM_10;

static QueueHandle_t s_timestamp_queue = nullptr;

void IRAM_ATTR GPIODriver::gpio_isr_handler(void* arg) {
    if (!s_timestamp_queue) return;

    uint32_t gpio_num = reinterpret_cast<uint32_t>(arg);
    
    common::SensorTimestamp event;
    event.timestamp_us = esp_timer_get_time();
    
    if (gpio_num == PIN_CADENCE) {
        event.sensor_id = common::SENSOR_ID_CADENCE;
    } else if (gpio_num == PIN_RPM) {
        event.sensor_id = common::SENSOR_ID_RPM;
    } else {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(s_timestamp_queue, &event, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void GPIODriver::init(QueueHandle_t timestamp_queue) {
    s_timestamp_queue = timestamp_queue;

    gpio_config_t io_conf = {};
    // Interrupt on falling edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // Bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << PIN_CADENCE) | (1ULL << PIN_RPM);
    // Set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // Enable pull-down mode as the opamp drives high
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    gpio_config(&io_conf);

    // Install gpio isr service
    gpio_install_isr_service(0);
    // Hook isr handler for specific gpio pin
    gpio_isr_handler_add(PIN_CADENCE, gpio_isr_handler, reinterpret_cast<void*>(PIN_CADENCE));
    gpio_isr_handler_add(PIN_RPM, gpio_isr_handler, reinterpret_cast<void*>(PIN_RPM));

    ESP_LOGI(TAG, "Initialized Reed Sensors on pins %d and %d", PIN_CADENCE, PIN_RPM);
}

} // namespace drivers
