#pragma once

#include <hardware/pio.h>
#include <pico/types.h>

typedef struct __attribute__((packed)) {
    uint8_t r, g, b;
} rgb_t;

rgb_t hsv2rgb(float h, float s, float v);

typedef struct {
    uint pin, count;
    PIO pio;
    int sm;
    int dma_ch;
    uint pio_off;

    rgb_t *buf;

    uint8_t brightness;
    bool gamma;

    uint32_t *tx_frame;
    bool tx_in_flight;
} sk6812_t;

#define SK6812_DECLARE(name, pin_, count_, pio_, sm_)                                              \
    static rgb_t name##_data_buf[(count_)];                                                        \
    static uint32_t name##_frame_buf[(count_)];                                                    \
    static const uint name##_led_count = (count_);                                                 \
    static const uint name##_pin = (pin_);                                                         \
    static sk6812_t name = {                                                                       \
        .pin = (pin_),                                                                             \
        .count = (count_),                                                                         \
        .pio = (pio_),                                                                             \
        .sm = (sm_),                                                                               \
        .dma_ch = -1,                                                                              \
        .pio_off = 0,                                                                              \
        .buf = name##_data_buf,                                                                    \
        .brightness = 255,                                                                         \
        .gamma = 0,                                                                                \
        .tx_frame = name##_frame_buf,                                                              \
        .tx_in_flight = 0,                                                                         \
    }

bool sk6812_begin(sk6812_t *);
void sk6812_end(sk6812_t *);
uint sk6812_size(sk6812_t *const);
void sk6812_clear(sk6812_t *);
void sk6812_set(sk6812_t *, uint, rgb_t);
void sk6812_brightness(sk6812_t *, uint8_t);
void sk6812_gamma(sk6812_t *, bool);
void sk6812_show(sk6812_t *);
void sk6812_show_async(sk6812_t *);
bool sk6812_busy(sk6812_t *const);
void sk6812_wait(sk6812_t *);
