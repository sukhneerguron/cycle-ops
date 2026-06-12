#pragma once
#include "models/RideModel.hpp"

struct ble_gatt_access_ctxt;

namespace consumers {

class BLEConsumer {
public:
    BLEConsumer(const models::RideModel* model);

    // Registers services before NimBLE host starts
    void init_services();

    // Should be called after BLEDriver indicates NimBLE is synced
    void start();

    // NimBLE callbacks and handles must be accessible to the static service definition
    static int gatt_svr_chr_access_csc(uint16_t conn_handle, uint16_t attr_handle,
                                       struct ::ble_gatt_access_ctxt *ctxt, void *arg);
    static int gatt_svr_chr_access_dis(uint16_t conn_handle, uint16_t attr_handle,
                                       struct ::ble_gatt_access_ctxt *ctxt, void *arg);
    static uint16_t s_csc_handle;

private:
    static void notify_task(void* arg);
    void run_notify();

    const models::RideModel* model_;
    static const models::RideModel* s_model_instance;
};

} // namespace consumers
