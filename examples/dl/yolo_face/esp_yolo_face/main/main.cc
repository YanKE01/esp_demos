#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "predict.h"
#include "string"
#include "hal_spiffs.h"
#include "esp_log.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "jpeg_decoder.h"

static const char *TAG = "Main";
char *filename = (char *)"/spiffs/resized_image.jpg";

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(hal_spiffs_init((char *)"/spiffs"));
    model_yoloface_init();

    // read pic
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        ESP_LOGI(TAG, "File %s size is %ld\n", filename, st.st_size);

        uint32_t filesize = (uint32_t)st.st_size; // read pic size
        float *file_buf = (float *)heap_caps_malloc(filesize + 1, MALLOC_CAP_SPIRAM);

        if (file_buf == NULL)
        {
            ESP_LOGI(TAG, "Malloc file buffer fail");
            return;
        }

        int f = open(filename, O_RDONLY);

        if (f > 0)
        {
            read(f, file_buf, filesize);
            size_t pic_buf_size = 64 * 64 * sizeof(uint32_t);
            uint8_t *pic_buf = (uint8_t *)heap_caps_malloc(pic_buf_size + 1, MALLOC_CAP_SPIRAM); // create out image buffer
            esp_jpeg_image_cfg_t jpeg_cfg = {
                .indata = (uint8_t *)file_buf,
                .indata_size = filesize,
                .outbuf = (uint8_t *)pic_buf,
                .outbuf_size = pic_buf_size,
                .out_format = JPEG_IMAGE_FORMAT_RGB888,
                .out_scale = JPEG_IMAGE_SCALE_0,
            };
            
            // decode jpg
            esp_jpeg_image_output_t outimage;
            esp_jpeg_decode(&jpeg_cfg, &outimage);
            ESP_LOGI(TAG, "%s size: %d x %d", filename, outimage.width, outimage.height);
            model_yoloface_predict((uint8_t *)pic_buf, outimage.width * outimage.height * 3);
            close(f);
        }

        free(file_buf);
    }
    else
    {
        ESP_LOGI(TAG, "Read Size Fail");
    }
}
