#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace models {

struct RideData {
    uint16_t current_wheel_rpm{0};
    uint32_t total_wheel_revolutions{0};
    
    uint16_t current_cadence{0};
    uint32_t total_crank_revolutions{0};
    
    // Time of last wheel/crank event, typically 1/1024s resolution for standard BLE CSC
    uint16_t last_wheel_event_time{0}; 
    uint16_t last_crank_event_time{0};
};

class RideModel {
public:
    RideModel() {
        mutex_ = xSemaphoreCreateMutex();
    }

    ~RideModel() {
        if (mutex_) vSemaphoreDelete(mutex_);
    }

    // Single Writer (CadenceService)
    void update(const RideData& new_data) {
        if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
            data_ = new_data;
            xSemaphoreGive(mutex_);
        }
    }

    // Multiple Readers (BLEConsumer, DisplayConsumer)
    RideData get() const {
        RideData copy;
        if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
            copy = data_;
            xSemaphoreGive(mutex_);
        }
        return copy;
    }

private:
    RideData data_;
    mutable SemaphoreHandle_t mutex_;
};

} // namespace models
