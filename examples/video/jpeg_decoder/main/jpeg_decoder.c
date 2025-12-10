#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_jpeg_common.h"
#include "esp_jpeg_dec.h"
#include "esp_log.h"

static const char *TAG = "JPEG_DECODER";
extern const uint8_t bus_jpg_start[] asm("_binary_bus_jpg_start");
extern const uint8_t bus_jpg_end[] asm("_binary_bus_jpg_end");

void app_main(void)
{
    jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
    config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;
    config.rotate = JPEG_ROTATE_0D;

    jpeg_dec_handle_t jpeg_dec = NULL;
    jpeg_error_t ret = jpeg_dec_open(&config, &jpeg_dec);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "jpeg_dec_open failed");
        return;
    }

    jpeg_dec_io_t *jpeg_io = calloc(1, sizeof(jpeg_dec_io_t));
    if (jpeg_io == NULL) {
        ESP_LOGE(TAG, "calloc jpeg_io failed");
        return;
    }

    jpeg_dec_header_info_t *out_info = calloc(1, sizeof(jpeg_dec_header_info_t));
    if (out_info == NULL) {
        ESP_LOGE(TAG, "calloc out_info failed");
        return;
    }

    jpeg_io->inbuf = (uint8_t *)bus_jpg_start;
    jpeg_io->inbuf_len = bus_jpg_end - bus_jpg_start;

    ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "jpeg_dec_parse_header failed");
        return;
    }

    uint8_t *out_buf = jpeg_calloc_align(out_info->width * out_info->height * 3, 16);
    if (out_buf == NULL) {
        ESP_LOGE(TAG, "calloc out_buf failed");
        return;
    }
    jpeg_io->outbuf = out_buf;

    ret = jpeg_dec_process(jpeg_dec, jpeg_io);
    if (ret != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "jpeg_dec_process failed");
        return;
    }

    ESP_LOGI(TAG, "jpeg_dec_process success");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
