#include "App.hpp"
#include "drivers/GPIODriver.hpp"
#include "drivers/BLEDriver.hpp"
#include "esp_log.h"

namespace application {

static const char* TAG = "App";

App* App::s_instance = nullptr;

App::App() 
    : cadence_service_(&ride_model_, nullptr), 
      ble_consumer_(&ride_model_),
      display_consumer_(ride_model_, display_driver_) {
    s_instance = this;
    
    // Create the queue for timestamp events from ISR to CadenceService
    sensor_queue_ = xQueueCreate(100, sizeof(common::SensorTimestamp));
    
    // Re-initialize CadenceService with the valid queue
    cadence_service_ = services::CadenceService(&ride_model_, sensor_queue_);
}

void App::start() {
    ESP_LOGI(TAG, "Initializing hardware drivers...");
    setup_hardware();

    ESP_LOGI(TAG, "Starting services...");
    cadence_service_.start();

    ESP_LOGI(TAG, "Starting consumers...");
    display_consumer_.start();

    // BLEConsumer will be started when NimBLE host is synced (via on_ble_sync)
}

void App::setup_hardware() {
    i2c_driver_.init();
    display_driver_.init(i2c_driver_.get_bus_handle());
    
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
