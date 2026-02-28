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

/* Vendor code for Microsoft OS 2.0 descriptor (WinUSB), must match BOS descriptor */
#define VENDOR_REQUEST_MICROSOFT     0x20

extern uint8_t const desc_ms_os_20[];
