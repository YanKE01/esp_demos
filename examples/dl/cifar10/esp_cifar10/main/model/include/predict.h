#pragma once

#include <cstdint>

/**
 * @brief cifar10 model init
 *
 */
void cifar10_model_init();

/**
 * @brief cifar10 model predict
 *
 * @param pic
 * @param size
 */
void cifar10_model_predict(const uint8_t *pic, int size);
