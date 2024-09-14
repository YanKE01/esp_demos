#pragma once

#include "stdio.h"

/**
 * @brief mnist init
 *
 */
void mnist_model_init();

/**
 * @brief mnist predict
 *
 * @param pic
 * @param size
 */
void mnist_model_predict(const uint8_t *pic, size_t size);
