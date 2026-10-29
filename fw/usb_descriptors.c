/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "usb_descriptors.h"
#include "bsp/board_api.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "common/tusb_types.h"
#include "device/usbd.h"
#include "tusb_config.h"

#define PID_MAP(itf, n) (CFG_TUD_##itf << (n))

#define USB_VID 0xCAFE
#define USB_BCD 0x0200
#define USB_PID                                                                                    \
    (0x4000 | PID_MAP(CDC, 0) | PID_MAP(MSC, 2) | PID_MAP(HID, 4) | PID_MAP(MIDI, 6) |             \
     PID_MAP(VENDOR, 8))

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,

    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x02,
    .iProduct = 0x01,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report_keyboard[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_HID_6KRO)),

    // NKRO
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
    HID_USAGE(HID_USAGE_DESKTOP_KEYBOARD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
    HID_REPORT_ID(REPORT_ID_HID_NKRO)

    // 8-bit modifiers
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),
    HID_USAGE_MIN(224),
    HID_USAGE_MAX(231),
    HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX(1),
    HID_REPORT_COUNT(8),
    HID_REPORT_SIZE(1),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // Reserved byte
    HID_REPORT_COUNT(1),
    HID_REPORT_SIZE(8),
    HID_INPUT(HID_CONSTANT),

    // NKRO bitfield: 120 keys (15 bytes)
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),
    HID_USAGE_MIN(0),
    HID_USAGE_MAX_N(255, 2),
    HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX(1),
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT_N(256, 2),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_COLLECTION_END,
};

uint8_t const desc_hid_report_consumer[] = {
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_HID_CONSUMER)),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    if (instance == 0) {
        return desc_hid_report_keyboard;
    }
    if (instance == 1) {
        return desc_hid_report_consumer;
    }

    return NULL;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

enum {
    STRID_LANGID = 0,
    STRID_PRODUCT,
    STRID_MANUFACTURER,
    STRID_SERIAL,
    STRID_HID,
    STRID_CDC,
    STRID_VENDOR,
};

char const *string_desc_arr[] = {
    [STRID_LANGID] = (const char[]){0x09, 0x04},
    [STRID_PRODUCT] = "SyntaxKBD",
    [STRID_MANUFACTURER] = "Thomas Kasper",
    [STRID_SERIAL] = NULL,
    [STRID_HID] = "SyntaxKBD HID",
    [STRID_CDC] = "SyntaxKBD CDC",
    [STRID_VENDOR] = "SyntaxKBD Vendor",
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum {
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID_KEYBOARD,
    ITF_NUM_HID_CONSUMER,
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN                                                                           \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN +       \
     CFG_TUD_VENDOR * TUD_VENDOR_DESC_LEN)

#define EPNUM_HID_KEYBOARD 0x81
#define EPNUM_HID_CONSUMER 0x82
#define EPNUM_CDC_NOTIF 0x83
#define EPNUM_CDC_OUT 0x04
#define EPNUM_CDC_IN 0x84
#define EPNUM_VENDOR_OUT 0x05
#define EPNUM_VENDOR_IN 0x85

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                          500),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, STRID_CDC, EPNUM_CDC_NOTIF, CFG_TUD_CDC_NOTIF_BUFSIZE,
                       EPNUM_CDC_OUT, EPNUM_CDC_IN, CFG_TUD_CDC_EP_BUFSIZE),

    TUD_HID_DESCRIPTOR(ITF_NUM_HID_KEYBOARD, STRID_HID, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(desc_hid_report_keyboard), EPNUM_HID_KEYBOARD, CFG_TUD_HID_EP_BUFSIZE,
                       1),

    TUD_HID_DESCRIPTOR(ITF_NUM_HID_CONSUMER, STRID_HID, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_consumer), EPNUM_HID_CONSUMER, CFG_TUD_HID_EP_BUFSIZE,
                       1),

    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, STRID_VENDOR, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN,
                          CFG_TUD_VENDOR_EP_BUFSIZE),
};

#if TUD_OPT_HIGH_SPEED
# error TUD_OPT_HIGH_SPEED is set
#endif

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration;
}

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    size_t chr_count;

    switch (index) {
    case STRID_LANGID:
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
        break;

    case STRID_SERIAL:
        chr_count = board_usb_get_serial(_desc_str + 1, 32);
        break;

    default:
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char *str = string_desc_arr[index];

        chr_count = strlen(str);
        size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
        if (chr_count > max_count)
            chr_count = max_count;

        for (size_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
        break;
    }

    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
