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
         "consumers/BLEConsumer.cpp"
         "drivers/BLEDriver.cpp"
         "drivers/GPIODriver.cpp"
         "services/CadenceService.cpp"
    INCLUDE_DIRS ${include_dirs}
    REQUIRES esp_timer driver esp_driver_gpio bt nvs_flash
)
