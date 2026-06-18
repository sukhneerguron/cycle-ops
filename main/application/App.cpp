#include "App.hpp"
#include "drivers/GPIODriver.hpp"
#include "drivers/BLEDriver.hpp"
#include "esp_log.h"

namespace application {

static const char* TAG = "App";

App* App::s_instance = nullptr;

App::App() 
    : cadence_service_(&ride_model_, nullptr, &event_bus_), 
      ble_consumer_(&ride_model_),
      display_task_(display_driver_, ride_model_, event_bus_) {
    s_instance = this;
    
    // Create the queue for timestamp events from ISR to CadenceService
    sensor_queue_ = xQueueCreate(100, sizeof(common::SensorTimestamp));
    
    // Re-initialise CadenceService with the valid queue
    cadence_service_ = services::CadenceService(&ride_model_, sensor_queue_, &event_bus_);
}

void App::start() {
    ESP_LOGI(TAG, "Initializing hardware drivers...");
    setup_hardware();

    ESP_LOGI(TAG, "Starting services...");
    cadence_service_.start();

    ESP_LOGI(TAG, "Starting consumers...");
    display_task_.start();

    // BLEConsumer will be started when NimBLE host is synced (via on_ble_sync)
}

void App::setup_hardware() {
    i2c_driver_.init();
    spi_driver_.init();
    display_driver_.init(spi_driver_.get_host());
    
    drivers::GPIODriver::init(sensor_queue_);
    drivers::BLEDriver::init("Cycle-Ops", 
        [this]() { ble_consumer_.init_services(); },
        App::on_ble_sync
    );
}

void App::on_ble_sync() {
    ESP_LOGI(TAG, "BLE Synced. Starting BLEConsumer...");
    if (s_instance) {
        s_instance->ble_consumer_.start();
    }
}

} // namespace application
