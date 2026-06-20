set(include_dirs
    "."
    "application"
    "common"
    "consumers"
    "drivers"
    "events"
    "models"
    "services"
    "ui"
)

idf_component_register(
    SRCS "main.cpp"
         "application/App.cpp"
         "consumers/ble/BLEConsumer.cpp"
         "drivers/BLEDriver.cpp"
         "drivers/GPIODriver.cpp"
         "drivers/I2CDriver.cpp"
         "drivers/SPIDriver.cpp"
         "drivers/DisplayDriver.cpp"
         "drivers/TouchDriver.cpp"
         "services/cadence/CadenceService.cpp"
         "ui/DisplayTask.cpp"
         "ui/screens/RideScreen.cpp"
         "ui/components/SpeedWidget.cpp"
         "ui/components/CadenceWidget.cpp"
    INCLUDE_DIRS ${include_dirs}
    REQUIRES esp_timer driver esp_driver_gpio bt nvs_flash esp_lcd esp_lcd_touch
)
