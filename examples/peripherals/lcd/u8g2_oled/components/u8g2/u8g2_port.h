#pragma once

#include <u8g2.h>
#include <u8x8.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t u8x8_byte_renesas_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include "U8g2lib.h"
#include <cstdio>

class U8G2_SSD1306_128X64_ESP_SPI : public U8G2{
public:
    U8G2_SSD1306_128X64_ESP_SPI();

    void printf(int x, int y, const char *format, ...);


    void print(const char str[]) {
        drawUTF8(tx, ty, str);
    }
    void print(uint8_t num) {
        printf(tx,ty,"%d",num);
    }
};

#endif
