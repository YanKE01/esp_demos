idf_component_register(SRCS "music_spectrum.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio esp_driver_i2s esp_timer led_strip)
