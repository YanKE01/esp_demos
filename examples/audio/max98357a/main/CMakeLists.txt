file(GLOB APP_SPIFFS "app_spiffs/*.c")
file(GLOB HAL_I2S "hal_i2s/*.c")

idf_component_register(SRCS "main.c" ${APP_SPIFFS} ${HAL_I2S}
                    INCLUDE_DIRS "." "app_spiffs/include/" "hal_i2s/include/")
