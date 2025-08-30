idf_component_register(SRCS "alarm.cc"
                    "wifi_reconfig.cc"
                    INCLUDE_DIRS "."
                    REQUIRES json nvs_flash esp_wifi)