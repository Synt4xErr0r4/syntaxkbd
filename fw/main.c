#include "bsp/board_api.h"
#include "config.h"
#include "device/usbd.h"
#include "hardware/watchdog.h"
#include "led.h"
#include "lighting.h"
#include "matrix.h"
#include "time.h"
#include "tusb_config.h"
#include "usb.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>

#define NDEBUG

int main() {
    board_init();
    tud_init(BOARD_TUD_RHPORT);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    stdio_init_all();
    watchdog_enable(5000, 1);
    ticks_init();

    uint64_t rgb_ticks = ticks_since_start();
    uint8_t h = 0;

    uint64_t matrix_ticks = ticks_since_start();

#ifndef NDEBUG
    while (ticks_since_start() <= TICKS_SEC(3)) {
        watchdog_update();

        tud_task();
    }
#endif

    printf("Starting SyntaxKBD...\n");

    if (watchdog_caused_reboot()) {
        printf("Warning: SyntaxKBD was rebooted by watchdog!\n");
    }

    lighting_init();
    config_load();
    matrix_init();
    led_init();

    syntaxkbd_config.flag_lighting = 1;
    syntaxkbd_config.effects[0].type = EFFECT_TYPE_WHITE;
    syntaxkbd_config.effects[0].direction = EFFECT_DIR_NORTHEAST;
    syntaxkbd_config.effects[0].hue = 0.f;
    syntaxkbd_config.effects[0].breathing = 0;
    syntaxkbd_config.effects[0].breathing_speed = 1.f;
    syntaxkbd_config.effects[0].brightness = .8f;
    syntaxkbd_config.effects[0].speed = 10.f;
    syntaxkbd_config.selected_effect = 0;
    lighting_dispatch(&((lcmd_t){.type = LCMD_RESTART}));

    printf("Setup done.\n");
    fflush(stdout);

    while (true) {
        watchdog_update();
        tud_task();
        usb_hid_task();
    }

    __builtin_unreachable();
}
