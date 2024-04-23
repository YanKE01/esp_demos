#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "predict.h"
#include "image_array.h"

extern "C" void app_main(void)
{
    cifar10_model_init();
    cifar10_model_predict(image_array, sizeof(image_array) / sizeof(image_array[0]));
}
