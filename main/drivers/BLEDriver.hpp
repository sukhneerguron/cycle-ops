#pragma once
#include <functional>

namespace drivers {

class BLEDriver {
public:
    using OnSyncCallback = std::function<void()>;
    using RegisterServicesCallback = std::function<void()>;

    // Initializes the NimBLE stack
    static void init(const char* device_name, RegisterServicesCallback register_svcs, OnSyncCallback on_sync);

private:
    static void nimble_host_task(void *param);
    static void on_sync_internal();
    
    static OnSyncCallback s_on_sync_callback;
};

} // namespace drivers
