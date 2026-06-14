set(include_dirs
    "."
    "application"
    "common"
    "consumers"
    "drivers"
    "events"
    "models"
    "services"
)

idf_component_register(
    SRCS "main.cpp"
         "application/App.cpp"
         "consumers/ble/BLEConsumer.cpp"
         "consumers/display/DisplayConsumer.cpp"
         "drivers/BLEDriver.cpp"
         "drivers/GPIODriver.cpp"
         "drivers/I2CDriver.cpp"
         "drivers/DisplayDriver.cpp"
         "services/cadence/CadenceService.cpp"
    INCLUDE_DIRS ${include_dirs}
    REQUIRES esp_timer driver esp_driver_gpio bt nvs_flash esp_lcd
)
