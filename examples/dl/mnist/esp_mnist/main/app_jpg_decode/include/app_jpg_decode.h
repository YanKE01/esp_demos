#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t *app_jpg_decode(char *path);
void app_rgb565_to_gray(uint8_t *pic_buf, uint8_t *gray_buf, int width, int height);
#ifdef __cplusplus
}
#endif
