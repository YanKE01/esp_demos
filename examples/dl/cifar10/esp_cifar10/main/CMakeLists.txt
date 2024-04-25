idf_component_register(SRCS "main.cc" "model/cifar10_model.cc" "model/predict.cc" "hal_spiffs/hal_spiffs.c"
                    INCLUDE_DIRS "." "model/include/" "hal_spiffs/include/")
