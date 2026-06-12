#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace drivers {

class GPIODriver {
public:
    // Initialize the GPIO pins for cadence (9) and rpm (10)
    // and attach the falling edge ISRs.
    // timestamp_queue is the FreeRTOS queue where ISRs will push common::SensorTimestamp events.
    static void init(QueueHandle_t timestamp_queue);

private:
    static void IRAM_ATTR gpio_isr_handler(void* arg);
};

} // namespace drivers
