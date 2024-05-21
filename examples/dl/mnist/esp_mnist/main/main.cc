#include <stdio.h>
#include "predict.h"
#include "jpeg_decoder.h"
#include "hal_spiffs.h"
#include "app_jpg_decode.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

uint8_t *pic_buf = NULL;
static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    // init spiffs
    ESP_ERROR_CHECK(hal_spiffs_init((char *)"/spiffs"));

    // init model
    mnist_model_init();

    // decode
    pic_buf = app_jpg_decode((char *)"/spiffs/2.jpeg");

    // conver to gray
    uint8_t *gray_buf = (uint8_t *)heap_caps_malloc(28 * 28 * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    if (gray_buf == NULL)
    {
        ESP_LOGI(TAG, "malloc gray buf fail");
        return;
    }
    app_rgb565_to_gray(pic_buf, gray_buf, 28, 28);

    // model predict
    mnist_model_predict(gray_buf, 28 * 28);

    // free memory
    free(gray_buf);
    if (pic_buf != NULL)
    {
        free(pic_buf);
    }
}
