idf_component_register(SRCS "main.cc" "model/mnist_model.cc" "model/predict.cc" "hal_spiffs/hal_spiffs.c" "app_jpg_decode/app_jpg_decode.c"
                    INCLUDE_DIRS "." "model/include/" "hal_spiffs/include/" "app_jpg_decode/include/")
