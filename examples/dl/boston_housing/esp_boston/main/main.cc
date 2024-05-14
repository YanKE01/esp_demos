#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "predict.h"
#include "math.h"

float value[] = {0.6339165, -0.48361546, 1.0283258, -0.25683275, 1.1578878, 0.19313958, 1.1104883, -1.0362827, 1.6758858, 1.5652875, 0.7844764, 0.22689421, 1.0446649};

extern "C" void app_main(void)
{
    boston_model_init();

    boston_model_predict(value, sizeof(value) / sizeof(value[0]));
}
