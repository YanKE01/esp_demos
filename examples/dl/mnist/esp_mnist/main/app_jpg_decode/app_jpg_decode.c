#include "app_jpg_decode.h"
#include "stdio.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "jpeg_decoder.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "JPG Decode";

uint8_t *app_jpg_decode(char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        ESP_LOGI(TAG, "File %s size is %ld\n", path, st.st_size);

        uint32_t filesize = (uint32_t)st.st_size; // read pic size
        float *file_buf = (float *)heap_caps_malloc(filesize + 1, MALLOC_CAP_SPIRAM);

        if (file_buf == NULL) {
            ESP_LOGI(TAG, "Malloc file buffer fail");
            return NULL;
        }

        int f = open(path, O_RDONLY);

        if (f > 0) {
            read(f, file_buf, filesize);
            size_t pic_buf_size = 28 * 28 * sizeof(uint16_t);
            uint8_t *pic_buf = (uint8_t *)heap_caps_malloc(pic_buf_size + 1, MALLOC_CAP_SPIRAM); // create out image buffer
            esp_jpeg_image_cfg_t jpeg_cfg = {
                .indata = (uint8_t *)file_buf,
                .indata_size = filesize,
                .outbuf = (uint8_t *)pic_buf,
                .outbuf_size = pic_buf_size,
                .out_format = JPEG_IMAGE_FORMAT_RGB565,
                .out_scale = JPEG_IMAGE_SCALE_0,
            };

            // // decode jpg
            esp_jpeg_image_output_t outimage;
            esp_jpeg_decode(&jpeg_cfg, &outimage);
            ESP_LOGI(TAG, "%s size: %d x %d", path, outimage.width, outimage.height);
            close(f);

            return pic_buf;
        }

        free(file_buf);
    } else {
        ESP_LOGI(TAG, "Read Size Fail");
    }

    return NULL;
}

void app_rgb565_to_gray(uint8_t *pic_buf, uint8_t *gray_buf, int width, int height)
{
    for (int i = 0; i < width * height; i++) {
        uint16_t pixel = ((uint16_t)pic_buf[2 * i + 1] << 8) | pic_buf[2 * i];

        // 提取RGB分量
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;

        // 扩展到8位精度
        r = (r * 255) / 31;
        g = (g * 255) / 63;
        b = (b * 255) / 31;

        // 计算灰度值
        uint8_t gray = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);

        // 存储灰度值
        gray_buf[i] = gray;
    }
}
