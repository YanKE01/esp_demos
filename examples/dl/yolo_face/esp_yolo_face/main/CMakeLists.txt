idf_component_register(SRCS "main.cc" "model/yoloface_int8.cc" "model/predict.cc" "hal_spiffs/hal_spiffs.c"
                    INCLUDE_DIRS "." "model/include/" "hal_spiffs/include/")
