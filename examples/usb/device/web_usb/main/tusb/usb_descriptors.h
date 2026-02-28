#pragma once

#include "tusb.h"

enum {
    ITF_NUM_VENDOR = 0,
    ITF_NUM_TOTAL,
};

enum {
    EPNUM_DEFAULT = 0,
    EPNUM_VENDOR,
    EPNUM_TOTAL
};

enum {
    VENDOR_REQUEST_WEBUSB = 1,
    VENDOR_REQUEST_MICROSOFT = 2
};

extern uint8_t const desc_ms_os_20[];
