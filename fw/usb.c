#include "usb.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "class/vendor/vendor_device.h"
#include "device/usbd.h"
#include "led.h"
#include "lighting.h"
#include "matrix.h"
#include "usb_descriptors.h"
#include <stdio.h>

static bool allow_wakeup = false;

void usb_hid_task(void) {
    if (!tud_hid_ready()) {
        return;
    }

    matrix_result_t res;
    memset(&res, 0, sizeof res);
    matrix_query(&res);

    tud_hid_keyboard_report(REPORT_ID_HID_6KRO, res.modifiers, res.array);

    if (tud_hid_n_get_protocol(0) == HID_PROTOCOL_REPORT) {
        {
            uint8_t report[34];
            report[0] = res.modifiers;
            report[1] = 0;
            memcpy(report + 2, res.bitmap, 32);

            tud_hid_report(REPORT_ID_HID_NKRO, report, 34);
        }

        {
            uint16_t report[1];
            report[0] = res.multimedia[0];

            tud_hid_n_report(1, REPORT_ID_HID_CONSUMER, report, 2);
        }
    }
}

void tud_suspend_cb(bool remote_wakeup_en) {
    allow_wakeup = remote_wakeup_en;

    lighting_dispatch(&((lcmd_t){.type = LCMD_OFF}));
}

void tud_resume_cb(void) {
    lighting_dispatch(&((lcmd_t){.type = LCMD_RESTART}));
}

////////////////////////////////////////////////////////////////////////////////
/// HID

void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t bufsize) {
    (void) instance;

    if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_HID_6KRO) {
        if (bufsize < 1) {
            return;
        }

        led_recv_state(buffer[0]);
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    // usb_send_report();

    if (report_id == REPORT_ID_HID_6KRO) {
        printf("%u:6KRO request: %u @ %u\n", instance, report_type, reqlen);
    } else if (report_id == REPORT_ID_HID_NKRO) {
        printf("%u:NKRO request: %u @ %u\n", instance, report_type, reqlen);
    } else {
        printf("%u:%u request: %u @ %u\n", instance, report_id, report_type, reqlen);
    }

    return 0;
}

void tud_hid_report_complete_cb(uint8_t instance, const uint8_t *report, uint16_t len) {
    ;
}

void tud_hid_report_failed_cb(uint8_t instance, hid_report_type_t report_type,
                              const uint8_t *report, uint16_t xferred_bytes) {
    printf("Failed to send HID report!\n");
}

////////////////////////////////////////////////////////////////////////////////
/// Vendor

void tud_vendor_rx_cb(uint8_t itf, const uint8_t *buffer, uint16_t bufsize) {
    printf("Rx'd vendor packet: %u = %u [%s]\n", itf, bufsize, buffer);
    tud_vendor_n_write_str(itf, "hi");
    tud_vendor_write_flush();

    tud_vendor_read_flush();
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    printf("Tx'd vendor packet: %u = %u\n", itf, sent_bytes);
}
