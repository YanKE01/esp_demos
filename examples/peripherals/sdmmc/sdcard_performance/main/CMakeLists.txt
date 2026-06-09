idf_component_register(SRCS "sdcard_performance.c"
                    INCLUDE_DIRS "."
                    REQUIRES fatfs esp_driver_sdmmc sdmmc esp_timer)
