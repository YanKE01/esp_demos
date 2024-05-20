#pragma once

#include <cstddef>
#include <cstdint>

void model_yoloface_init();
void model_yoloface_predict(uint8_t *pic,size_t size);