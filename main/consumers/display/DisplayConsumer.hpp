#pragma once

#include "RideModel.hpp"
#include "DisplayDriver.hpp"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace consumers {

class DisplayConsumer {
public:
    DisplayConsumer(models::RideModel& ride_model, drivers::DisplayDriver& display_driver);
    ~DisplayConsumer();

    void start();

private:
    static void consumer_task(void* arg);
    void update_ui();
    void create_ui();

    models::RideModel& ride_model_;
    drivers::DisplayDriver& display_driver_;
    TaskHandle_t task_handle_{nullptr};

    lv_obj_t* screen_{nullptr};
    lv_obj_t* rpm_label_{nullptr};
    lv_obj_t* cadence_label_{nullptr};
    lv_obj_t* revs_label_{nullptr};
};

} // namespace consumers
