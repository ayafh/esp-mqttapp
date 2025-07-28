idf_component_register(
    SRCS "mqttsensor.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif mqtt nvs_flash esp_adc driver 
)
