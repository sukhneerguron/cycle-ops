#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "models/RideModel.hpp"

namespace services {

class CadenceService {
public:
    // model: The shared RideModel where we will publish our calculations
    // queue: The Queue where we receive SensorTimestamp events from the ISR
    CadenceService(models::RideModel* model, QueueHandle_t queue);
    ~CadenceService();

    void start();

private:
    static void task_entry(void* arg);
    void run();
    void process_event(uint8_t sensor_id, uint64_t timestamp_us);

    models::RideModel* model_;
    QueueHandle_t queue_;
    TaskHandle_t task_handle_{nullptr};

    // Debouncing config
    static constexpr uint64_t DEBOUNCE_TIME_US = 20000; // 20ms

    // State for Cadence
    uint64_t last_cadence_time_us_{0};
    uint32_t total_crank_revolutions_{0};
    uint16_t current_cadence_{0}; // rpm

    // State for RPM
    uint64_t last_rpm_time_us_{0};
    uint32_t total_wheel_revolutions_{0};
    uint16_t current_rpm_{0}; // rpm
};

} // namespace services
