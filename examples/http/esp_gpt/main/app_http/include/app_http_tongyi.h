#pragma once

#include "esp_http_client.h"

extern QueueHandle_t xTongyiQuestion;

void app_http_ask_tongyi_task(void *pvParameters);