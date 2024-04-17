#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1


extern EventGroupHandle_t app_wifi_event_group;

/**
 * @brief app wifi connect
 *
 * @param ssid
 * @param pswd
 */
void app_wifi_init(char *ssid, char *pswd);
