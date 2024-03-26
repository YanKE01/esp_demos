#pragma once

#include "tinyusb.h"

extern const char *hid_device_audio_string_descriptor[5];
extern tusb_desc_device_t hid_aduio_device_descriptor;
extern const uint8_t hid_device_audio_configuration_descriptor[];
extern const uint8_t hid_device_audio_ctrl_report_descriptor[];

/**
 * @brief test audio ctrl
 * 
 * @return true 
 * @return false 
 */
bool hid_device_audio_ctrl();