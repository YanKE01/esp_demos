#include "U8g2lib.h"
#include "u8g2_port.h"


extern "C" uint8_t  u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8,
                               uint8_t msg,
                               uint8_t arg_int,
                               void* arg_ptr);

extern "C" uint8_t  u8g2_esp32_spi_byte_cb(u8x8_t* u8x8,
                                           uint8_t msg,
                                           uint8_t arg_int,
                                           void* arg_ptr);

extern "C" uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8,
                                     uint8_t msg,
                                     uint8_t arg_int,
                                     void* arg_ptr);

U8G2_SSD1306_128X64_ESP_SPI::U8G2_SSD1306_128X64_ESP_SPI() : U8G2() {
    u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);
}


void U8G2_SSD1306_128X64_ESP_SPI::printf(int x, int y, const char *format, ...) {
    char buffer[128];  //缓冲
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);  // 使用 vsnprintf 将格式化的字符串写入缓冲区
    va_end(args);

    u8g2_DrawUTF8(&u8g2, x, y, buffer);  // 将缓冲区的内容传递给 u8g2_DrawStr 函数进行显示
}

