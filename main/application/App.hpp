#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "common/Types.hpp"
#include "events/EventBus.hpp"
#include "models/RideModel.hpp"
#include "services/CadenceService.hpp"
#include "consumers/BLEConsumer.hpp"
#include "drivers/I2CDriver.hpp"
#include "drivers/DisplayDriver.hpp"
#include "consumers/DisplayConsumer.hpp"

namespace application {

class App {
public:
    App();
    void start();

private:
    void setup_hardware();
    static void on_ble_sync();

    // The single instance of App for BLE callbacks
    static App* s_instance;

    events::EventBus event_bus_;
    models::RideModel ride_model_;

    QueueHandle_t sensor_queue_{nullptr};

    services::CadenceService cadence_service_;
    consumers::BLEConsumer ble_consumer_;
    
    drivers::I2CDriver i2c_driver_;
    drivers::DisplayDriver display_driver_;
    consumers::DisplayConsumer display_consumer_;
};

} // namespace application
