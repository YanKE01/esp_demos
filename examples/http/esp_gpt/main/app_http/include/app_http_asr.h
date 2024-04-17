#pragma once

#include "esp_http_client.h"

extern esp_http_client_handle_t asr_client;

void app_http_asr(int time);