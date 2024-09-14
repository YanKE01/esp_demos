#pragma once

#include "tinyusb.h"

extern const char *hid_keyboard_string_descriptor[5];
extern tusb_desc_device_t hid_keyboard_descriptor;
extern const uint8_t hid_keyboard_configuration_descriptor[];

/**
 * @brief keyboard test
 *
 */
void keyboard_win_test();
