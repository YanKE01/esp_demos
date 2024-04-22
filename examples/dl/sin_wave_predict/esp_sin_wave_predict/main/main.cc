#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "predict.h"
#include "math.h"

extern "C" void app_main(void)
{
    float value = 0.5f * M_PI;
    sin_wave_model_init();

    while (1)
    {
        sin_wave_model_predict(value);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
