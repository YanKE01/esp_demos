idf_component_register(SRCS "main.cc" "model/mfcc_model_quantized.cc" "model/predict.cc" "hal/hal_i2s.c" "hal/hal_sd.c"
                    INCLUDE_DIRS "." "model/include/" "hal/include/")
