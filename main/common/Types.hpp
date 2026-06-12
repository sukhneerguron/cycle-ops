#pragma once
#include <cstdint>

namespace common {

struct SensorTimestamp {
    uint8_t sensor_id; // 0 for Cadence, 1 for RPM
    uint64_t timestamp_us;
};

// IDs for the reed sensors
constexpr uint8_t SENSOR_ID_CADENCE = 0;
constexpr uint8_t SENSOR_ID_RPM = 1;

} // namespace common
