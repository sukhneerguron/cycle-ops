#include "BLEConsumer.hpp"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace consumers {

static const char* TAG = "BLEConsumer";

// BLE UUIDs for standard Cycling Speed and Cadence Service
static const ble_uuid16_t GATT_SVC_CSC_UUID = BLE_UUID16_INIT(0x1816);
static const ble_uuid16_t GATT_CHR_CSC_MEASUREMENT_UUID = BLE_UUID16_INIT(0x2A5B);
static const ble_uuid16_t GATT_CHR_CSC_FEATURE_UUID = BLE_UUID16_INIT(0x2A5C);
static const ble_uuid16_t GATT_CHR_SENSOR_LOCATION_UUID = BLE_UUID16_INIT(0x2A5D);

// BLE UUIDs for Device Information Service
static const ble_uuid16_t GATT_SVC_DIS_UUID = BLE_UUID16_INIT(0x180A);
static const ble_uuid16_t GATT_CHR_MANUFACTURER_NAME_UUID = BLE_UUID16_INIT(0x2A29);
static const ble_uuid16_t GATT_CHR_SERIAL_NUMBER_UUID = BLE_UUID16_INIT(0x2A25);

const models::RideModel* BLEConsumer::s_model_instance = nullptr;
uint16_t BLEConsumer::s_csc_handle = 0;

BLEConsumer::BLEConsumer(const models::RideModel* model) : model_(model) {
    s_model_instance = model;
}

int BLEConsumer::gatt_svr_chr_access_csc(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ::ble_gatt_access_ctxt *ctxt, void *arg) {
    const ble_uuid_t* uuid = ctxt->chr->uuid;

    if (ble_uuid_cmp(uuid, &GATT_CHR_CSC_FEATURE_UUID.u) == 0) {
        // Feature: Wheel Revolution Data Supported (bit 0) | Crank Revolution Data Supported (bit 1)
        uint16_t features = 0x0003; 
        int rc = os_mbuf_append(ctxt->om, &features, sizeof(features));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &GATT_CHR_SENSOR_LOCATION_UUID.u) == 0) {
        // Sensor Location: 13 = Rear Hub
        uint8_t location = 13;
        int rc = os_mbuf_append(ctxt->om, &location, sizeof(location));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

int BLEConsumer::gatt_svr_chr_access_dis(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ::ble_gatt_access_ctxt *ctxt, void *arg) {
    const ble_uuid_t* uuid = ctxt->chr->uuid;

    if (ble_uuid_cmp(uuid, &GATT_CHR_MANUFACTURER_NAME_UUID.u) == 0) {
        const char* mfg_name = "Garmin";
        int rc = os_mbuf_append(ctxt->om, mfg_name, strlen(mfg_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &GATT_CHR_SERIAL_NUMBER_UUID.u) == 0) {
        const char* serial = "SN-00000001";
        int rc = os_mbuf_append(ctxt->om, serial, strlen(serial));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &GATT_SVC_CSC_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &GATT_CHR_CSC_MEASUREMENT_UUID.u,
                .access_cb = BLEConsumer::gatt_svr_chr_access_csc,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &BLEConsumer::s_csc_handle,
            },
            {
                .uuid = &GATT_CHR_CSC_FEATURE_UUID.u,
                .access_cb = BLEConsumer::gatt_svr_chr_access_csc,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = &GATT_CHR_SENSOR_LOCATION_UUID.u,
                .access_cb = BLEConsumer::gatt_svr_chr_access_csc,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {0, } // No more characteristics in this service
        },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &GATT_SVC_DIS_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &GATT_CHR_MANUFACTURER_NAME_UUID.u,
                .access_cb = BLEConsumer::gatt_svr_chr_access_dis,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = &GATT_CHR_SERIAL_NUMBER_UUID.u,
                .access_cb = BLEConsumer::gatt_svr_chr_access_dis,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {0, } // No more characteristics in this service
        },
    },
    {0, } // No more services
};

#pragma GCC diagnostic pop

void BLEConsumer::init_services() {
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT services, rc: %d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services, rc: %d", rc);
        return;
    }
}

void BLEConsumer::start() {
    // Start advertising
    ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    uint8_t own_addr_type;
    ble_hs_id_infer_auto(0, &own_addr_type);

    // Set advertisement fields (Device Name, UUIDs, Flags)
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    // Advertising flags: Discoverable + BLE only (no BR/EDR)
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Set the device name in the advertisement
    const char *name = ble_svc_gap_device_name();
    if (name) {
        fields.name = (uint8_t *)name;
        fields.name_len = strlen(name);
        fields.name_is_complete = 1;
    }

    // Advertise the CSC service UUID
    fields.uuids16 = (ble_uuid16_t*)&GATT_SVC_CSC_UUID;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement fields; rc=%d", rc);
        return;
    }

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting advertising; rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Started Advertising");
    }

    // Start notification task
    xTaskCreate(notify_task, "BLENotify", 4096, this, 4, nullptr);
}

void BLEConsumer::notify_task(void* arg) {
    auto* self = static_cast<BLEConsumer*>(arg);
    self->run_notify();
}

void BLEConsumer::run_notify() {
    while (true) {
        // Send a notification every 1 second if connected (simplification)
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        if (!s_model_instance || s_csc_handle == 0) continue;

        models::RideData data = s_model_instance->get();

        // CSC Measurement format:
        // Flags (8-bit)
        // Cumulative Wheel Revolutions (32-bit)
        // Last Wheel Event Time (16-bit)
        // Cumulative Crank Revolutions (16-bit)
        // Last Crank Event Time (16-bit)
        
        uint8_t flags = 0x03; // Bit 0: Wheel data present, Bit 1: Crank data present
        struct os_mbuf *om = ble_hs_mbuf_from_flat(NULL, 0);
        if (!om) continue;

        os_mbuf_append(om, &flags, sizeof(flags));
        os_mbuf_append(om, &data.total_wheel_revolutions, sizeof(data.total_wheel_revolutions));
        os_mbuf_append(om, &data.last_wheel_event_time, sizeof(data.last_wheel_event_time));
        
        uint16_t crank_revs_16 = data.total_crank_revolutions & 0xFFFF; // CSC specifies 16-bit for crank
        os_mbuf_append(om, &crank_revs_16, sizeof(crank_revs_16));
        os_mbuf_append(om, &data.last_crank_event_time, sizeof(data.last_crank_event_time));

        // For simplicity, notify conn_handle 1.
        ble_gatts_notify_custom(1, s_csc_handle, om);
    }
}

} // namespace consumers
