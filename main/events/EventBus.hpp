#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace events {

class EventBus {
public:
    static constexpr EventBits_t RIDE_STARTED     = BIT0;
    static constexpr EventBits_t RIDE_STOPPED     = BIT1;
    static constexpr EventBits_t BLE_CONNECTED    = BIT2;
    static constexpr EventBits_t BLE_DISCONNECTED = BIT3;
    static constexpr EventBits_t RIDE_DATA_UPDATED = BIT4;
    static constexpr EventBits_t TOUCH_INTERRUPT  = BIT5;

    EventBus() {
        event_group_ = xEventGroupCreate();
    }

    ~EventBus() {
        if (event_group_) {
            vEventGroupDelete(event_group_);
        }
    }

    void publish(EventBits_t bitsToSet) {
        xEventGroupSetBits(event_group_, bitsToSet);
    }

    void publishFromISR(EventBits_t bitsToSet) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xEventGroupSetBitsFromISR(event_group_, bitsToSet, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }

    void clear(EventBits_t bitsToClear) {
        xEventGroupClearBits(event_group_, bitsToClear);
    }

    EventBits_t waitAny(EventBits_t bitsToWaitFor, TickType_t ticksToWait = portMAX_DELAY) {
        return xEventGroupWaitBits(event_group_, bitsToWaitFor, pdFALSE, pdFALSE, ticksToWait);
    }

private:
    EventGroupHandle_t event_group_;
};

} // namespace events
