file(GLOB APP_SRCS "app_wifi/*.c" "app_http/*.c" "hal_i2s/*.c" "app_spiffs/*.c")
set(INCLUDE_DIRS "app_http/include/" "app_wifi/include/" "hal_i2s/include/" "app_spiffs/include/")

idf_component_register(SRCS "main.c" ${APP_SRCS}
                    INCLUDE_DIRS "." ${INCLUDE_DIRS})
