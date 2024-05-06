file(GLOB LV_SRCS "app_ui/*.c")


idf_component_register(SRCS "main.c" ${LV_SRCS} "hal_spiffs/hal_spiffs.c" 
                    INCLUDE_DIRS "." "app_ui/" "hal_spiffs/include/"
                    REQUIRES spiffs)
