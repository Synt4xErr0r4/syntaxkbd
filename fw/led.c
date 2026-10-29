#include "led.h"
#include "class/hid/hid.h"
#include "config.h"
#include "sk6812.h"
#include <hardware/gpio.h>
#include <hardware/structs/watchdog.h>
#include <hardware/watchdog.h>
#include <pico/platform/common.h>
#include <pico/time.h>

SK6812_DECLARE(leds, 10, 4, pio1, -1);

void led_init(void) {
    sk6812_begin(&leds);
    sk6812_gamma(&leds, 1);
    sk6812_brightness(&leds, 10);

    led_refresh();
}

static uint8_t current_state = 0;

void led_recv_state(uint8_t state) {
    current_state = state;

    sk6812_clear(&leds);

    syntaxkbd_config.numlock_color = syntaxkbd_config.capslock_color =
        syntaxkbd_config.scrolllock_color = syntaxkbd_config.winlock_color = (rgb_t){255, 0, 0};

    if (syntaxkbd_config.flag_winlock) {
        sk6812_set(&leds, 0, syntaxkbd_config.winlock_color);
    }

    if ((state & KEYBOARD_LED_SCROLLLOCK)) {
        sk6812_set(&leds, 1, syntaxkbd_config.scrolllock_color);
    }
    if ((state & KEYBOARD_LED_CAPSLOCK)) {
        sk6812_set(&leds, 2, syntaxkbd_config.capslock_color);
    }
    if ((state & KEYBOARD_LED_NUMLOCK)) {
        sk6812_set(&leds, 3, syntaxkbd_config.numlock_color);
    }

    sk6812_show(&leds);
}

void led_refresh(void) {
    led_recv_state(current_state);
}
