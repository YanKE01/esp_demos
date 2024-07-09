idf_component_register(SRCS "main.cc" "model/mfcc_model_quantized.cc" "model/predict.cc" "hal_spiffs/hal_spiffs.c"
                    INCLUDE_DIRS "." "model/include/" "hal_spiffs/include/")
