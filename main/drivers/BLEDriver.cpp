#include "BLEDriver.hpp"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

namespace drivers {

static const char* TAG = "BLEDriver";
BLEDriver::OnSyncCallback BLEDriver::s_on_sync_callback = nullptr;

void BLEDriver::on_sync_internal() {
    ESP_LOGI(TAG, "NimBLE host synced");
    
    // Make sure we have a valid address
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error generating BLE address");
    }

    if (s_on_sync_callback) {
        s_on_sync_callback();
    }
}

void BLEDriver::nimble_host_task(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    // This function will return only when nimble_port_stop() is executed
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BLEDriver::init(const char* device_name, RegisterServicesCallback register_svcs, OnSyncCallback on_sync) {
    s_on_sync_callback = on_sync;

    // Initialize the NimBLE port
    ESP_ERROR_CHECK(nimble_port_init());

    // Set NimBLE host callbacks
    ble_hs_cfg.sync_cb = on_sync_internal;

    // Initialize default GAP/GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Set device name
    int rc = ble_svc_gap_device_name_set(device_name);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set device name, rc: %d", rc);
    }

    if (register_svcs) {
        register_svcs();
    }

    // Start the NimBLE host task
    nimble_port_freertos_init(nimble_host_task);
    
    ESP_LOGI(TAG, "NimBLE stack initialized");
}

} // namespace drivers
