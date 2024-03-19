file(GLOB APP_WIFI_SRCS "app_wifi/*.c")
file(GLOB HAL_SPIFFS_SRCS "hal_spiffs/*.c")
file(GLOB APP_BAIDU_SRCS "app_baidu/*.c")


idf_component_register(SRCS "main.c" ${APP_WIFI_SRCS} ${HAL_SPIFFS_SRCS} ${APP_BAIDU_SRCS}
                    INCLUDE_DIRS "." "app_wifi/include/" "hal_spiffs/include/" "app_baidu/include/")
