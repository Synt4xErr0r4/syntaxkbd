#include "lighting.h"
#include "config.h"
#include "matrix.h"
#include "sk6812.h"
#include "time.h"
#include <math.h>
#include <pico/flash.h>
#include <pico/multicore.h>
#include <pico/sem.h>
#include <pico/time.h>
#include <pico/util/queue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

SK6812_DECLARE(rgb0, 7, 16, pio0, -1);
SK6812_DECLARE(rgb1, 6, 21, pio0, -1);
SK6812_DECLARE(rgb2, 5, 22, pio0, -1);
SK6812_DECLARE(rgb3, 4, 18, pio0, -1);
SK6812_DECLARE(rgb4, 3, 19, pio1, -1);
SK6812_DECLARE(rgb5, 2, 14, pio1, -1);

static sk6812_t *RGB[NROW] = {
    &rgb0, &rgb1, &rgb2, &rgb3, &rgb4, &rgb5,
};

typedef struct {
    float x, y;
} pos_t;

// physical keycode positions
static const pos_t POS0[] = {
    {1.75, 0},  {3.75, 0},  {4.75, 0},  {5.75, 0},  {6.75, 0},  {8.25, 0}, {9.25, 0}, {10.25, 0},
    {11.25, 0}, {12.75, 0}, {13.75, 0}, {14.75, 0}, {15.75, 0}, {17.0, 0}, {18.0, 0}, {19.0, 0},
};
static const pos_t POS1[] = {
    {0.5, 1.5},  {1.75, 1.5}, {2.75, 1.5}, {3.75, 1.5},  {4.75, 1.5},  {5.75, 1.5},  {6.75, 1.5},
    {7.75, 1.5}, {8.75, 1.5}, {9.75, 1.5}, {10.75, 1.5}, {11.75, 1.5}, {12.75, 1.5}, {13.75, 1.5},
    {17.0, 1.5}, {18.0, 1.5}, {19.0, 1.5}, {20.25, 1.5}, {21.25, 1.5}, {22.25, 1.5}, {23.25, 1.5},
};
static const pos_t POS2[] = {
    {0.5, 2.5},   {2.0, 2.5},   {3.25, 2.5},  {4.25, 2.5},  {5.25, 2.5},  {6.25, 2.5},
    {7.25, 2.5},  {8.25, 2.5},  {9.25, 2.5},  {10.25, 2.5}, {11.25, 2.5}, {12.25, 2.5},
    {13.25, 2.5}, {14.25, 2.5}, {15.25, 1.5}, {17.0, 2.5},  {18.0, 2.5},  {19.0, 2.5},
    {20.25, 2.5}, {21.25, 2.5}, {22.25, 2.5}, {23.25, 2.5},
};
static const pos_t POS3[] = {
    {0.5, 3.5},  {2.125, 3.5}, {3.5, 3.5},    {4.5, 3.5},   {5.5, 3.5},   {6.5, 3.5},
    {7.5, 3.5},  {8.5, 3.5},   {9.5, 3.5},    {10.5, 3.5},  {11.5, 3.5},  {12.5, 3.5},
    {13.5, 3.5}, {14.5, 3.5},  {15.625, 3.0}, {20.25, 3.5}, {21.25, 3.5}, {22.25, 3.5},
};
static const pos_t POS4[] = {
    {0.5, 4.5},  {1.875, 4.5}, {3.0, 4.5},   {4.0, 4.5},   {5.0, 4.5},   {6.0, 4.5},  {7.0, 4.5},
    {8.0, 4.5},  {9.0, 4.5},   {10.0, 4.5},  {11.0, 4.5},  {12.0, 4.5},  {13.0, 4.5}, {14.875, 4.5},
    {18.0, 4.5}, {20.25, 4.5}, {21.25, 4.5}, {22.25, 4.5}, {23.25, 4.5},
};
static const pos_t POS5[] = {
    {0.5, 5.5},    {1.875, 5.5},  {3.125, 5.5},  {4.375, 5.5},  {8.125, 5.5},
    {11.875, 5.5}, {13.125, 5.5}, {14.375, 5.5}, {15.625, 5.5}, {17.0, 5.5},
    {18.0, 5.5},   {19.0, 5.5},   {20.75, 5.5},  {22.25, 5.5},
};

static const pos_t *const POSITIONS[] = {
    POS0, POS1, POS2, POS3, POS4, POS5,
};

typedef struct {
    uint64_t last_ticks;
    bool render_top;
} effect_state_t;

static queue_t cmd_queue;
static semaphore_t cmd_done;

static effect_t active_effect;
static effect_state_t active_effect_state;

static effect_t preview_effect;
static effect_state_t preview_effect_state;

void lighting_off(bool commit) {
    active_effect.type = EFFECT_TYPE_OFF;
    preview_effect.type = EFFECT_TYPE_OFF;

    memset(&active_effect_state, 0, sizeof(effect_state_t));
    memset(&preview_effect_state, 0, sizeof(effect_state_t));

    for (uint8_t group = 0; group < NROW; ++group) {
        sk6812_clear(RGB[group]);
        if (commit) {
            sk6812_show(RGB[group]);
        }
    }
}

void lighting_reset(void) {
    lighting_off(true);

    if (!syntaxkbd_config.flag_lighting) {
        return;
    }

    uint32_t selection = syntaxkbd_config.selected_effect;
    if (selection >= CONFIG_NEFFECT) {
        return;
    }

    effect_t const *e = &syntaxkbd_config.effects[selection];
    if (e->type == EFFECT_TYPE_OFF) {
        return;
    }

    memcpy(&active_effect, e, sizeof(effect_t));

#define CLAMP(val, min, max) (val) = ((val) > (max) ? (max) : (val) < (min) ? (min) : (val))

    CLAMP(active_effect.brightness, 0.f, 1.f);
    CLAMP(active_effect.breathing_speed, 0.f, 1.f);
    CLAMP(active_effect.speed, 0.f, 1.f);
    CLAMP(active_effect.hue, 0.f, 360.f);

    printf("Started lighting effect: %u (brightness %f, breathing %s @ %f, speed %f, hue %f)\n",
           active_effect.type, active_effect.brightness, active_effect.breathing ? "on" : "off",
           active_effect.breathing_speed, active_effect.speed, active_effect.hue);
}

typedef struct {
    uint8_t row;
    uint8_t from, to;
} lighting_range_t;

static void lighting_play(lighting_range_t const ranges[], size_t const nrange,
                          effect_t const *const effect, effect_state_t *const state) {
    uint64_t const now = ticks_since_start();
    uint64_t const delta = now - state->last_ticks;
    state->last_ticks = now;

    float const now_sec = now / 1000000.f;
    float const delta_sec = delta / 1000000.f;

    float value = effect->brightness;

    if (effect->breathing) {
        value *= (sinf(now_sec * effect->breathing_speed) + 1.f) * .4f + .2f;
    }

    switch (active_effect.type) {
    case EFFECT_TYPE_STATIC: {
        rgb_t const color = hsv2rgb(effect->hue, 1.f, value);

        for (uint i = 0; i < nrange; ++i) {
            lighting_range_t const *const range = &ranges[i];
            sk6812_t *const rgb = RGB[range->row];

            for (uint led = range->from; led < range->to; ++led) {
                sk6812_set(rgb, led, color);
            }

            sk6812_show(rgb);
        }

        break;
    }

    case EFFECT_TYPE_RAINBOW: {
        break;
    }

    case EFFECT_TYPE_WAVE: {
        for (uint i = 0; i < nrange; ++i) {
            lighting_range_t const *const range = &ranges[i];
            sk6812_t *const rgb = RGB[range->row];
            pos_t const *const positions = POSITIONS[range->row];

            for (uint led = range->from; led < range->to; ++led) {
                pos_t pos = positions[led];

                float hue = 0.f;

                if (effect->direction & 0b0010) {
                    hue += (effect->direction & 0b0001) ? -pos.y : pos.y;
                }
                if (effect->direction & 0b1000) {
                    hue += (effect->direction & 0b0100) ? -pos.x : pos.x;
                }

                hue *= 20;
                hue += 360 * now_sec * effect->speed;

                while (hue < 0.f) {
                    hue += 360.f;
                }
                while (hue > 360.f) {
                    hue -= 360.f;
                }

                sk6812_set(rgb, led, hsv2rgb(hue, 1.f, value));
            }

            sk6812_show(rgb);
        }
        break;
    }

    case EFFECT_TYPE_WHITE: {
        uint8_t const v = 255 * effect->brightness;
        rgb_t const color = {v, v, v};

        for (uint i = 0; i < nrange; ++i) {
            lighting_range_t const *const range = &ranges[i];
            sk6812_t *const rgb = RGB[range->row];

            for (uint led = range->from; led < range->to; ++led) {
                sk6812_set(rgb, led, color);
            }

            sk6812_show(rgb);
        }

        break;
    }

    default: {
        for (uint row = 0; row < NROW; ++row) {
            sk6812_clear(RGB[row]);
            sk6812_show(RGB[row]);
        }
        break;
    }
    }
}

static lighting_range_t const range_full[] = {
    {0, 0, rgb0_led_count}, {1, 0, rgb1_led_count}, {2, 0, rgb2_led_count},
    {3, 0, rgb3_led_count}, {4, 0, rgb4_led_count}, {5, 0, rgb5_led_count},
};

static lighting_range_t const range_top[] = {
    {0, 0, rgb0_led_count},
    {1, 0, rgb1_led_count},
    {2, 0, rgb2_led_count},
};

static lighting_range_t const range_bottom[] = {
    {3, 0, rgb3_led_count},
    {4, 0, rgb4_led_count},
    {5, 0, rgb5_led_count},
};

static lighting_range_t const range_preview[] = {
    {2, 3, 9},
    {3, 3, 9},
    {4, 4, 10},
};

void lighting_tick(void) {
    if (active_effect.type != EFFECT_TYPE_OFF) {

        /*
        if (active_effect_state.render_top ^= 1) {
            // for (uint i = 0; i < 3; ++i) {
            //     sk6812_clear(RGB[3 + i]);
            //     sk6812_show(RGB[3 + i]);
            // }

            lighting_play(range_top, sizeof range_top / sizeof *range_top, &active_effect,
                          &active_effect_state);
        } else {
            // for (uint i = 0; i < 3; ++i) {
            //     sk6812_clear(RGB[i]);
            //     sk6812_show(RGB[i]);
            // }

            lighting_play(range_bottom, sizeof range_bottom / sizeof *range_bottom, &active_effect,
                          &active_effect_state);
        }
        */

        lighting_play(range_full, sizeof range_full / sizeof *range_full, &active_effect,
                      &active_effect_state);

    } else if (preview_effect.type != EFFECT_TYPE_OFF) {
        lighting_play(range_preview, sizeof range_preview / sizeof *range_preview, &preview_effect,
                      &preview_effect_state);
    }
}

void lighting_highlight(lcmd_highlight_t *highlights, size_t n) {
    lighting_off(false);

    while (n--) {
        lcmd_highlight_t highlight = *highlights++;

        if (highlight.row >= NROW) {
            continue;
        }

        sk6812_set(RGB[highlight.row], highlight.col, highlight.color);
    }

    for (uint i = 0; i < NROW; ++i) {
        sk6812_show(RGB[i]);
    }
}

void lighting_task(void) {
    printf("Starting lighting task on second core...\n");

    hard_assert(flash_safe_execute_core_init());

    for (uint i = 0; i < NROW; ++i) {
        sk6812_t *rgb = RGB[i];
        sk6812_begin(rgb);
        sk6812_gamma(rgb, 1);
        sk6812_brightness(rgb, 128);
    }

    lighting_off(true);

    while (1) {
        if (queue_is_empty(&cmd_queue)) {
            lighting_tick();
            sleep_ms(1);
        } else {
            lcmd_t cmd;
            queue_remove_blocking(&cmd_queue, &cmd);

            printf("Received lighting command: %u\n", cmd.type);

            switch (cmd.type) {
            case LCMD_RESTART:
                lighting_reset();
                break;

            case LCMD_OFF:
                lighting_off(true);
                break;

            case LCMD_HIGHLIGHT:
                lighting_highlight(cmd.highlight.highlights, cmd.highlight.nhighlights);
                break;

            case LCMD_PREVIEW:
                memcpy(&preview_effect, &cmd.preview.effect, sizeof(effect_t));
                break;
            }

            sem_release(&cmd_done);
        }
    }
}

void lighting_init(void) {
    queue_init(&cmd_queue, sizeof(lcmd_t), 1);
    sem_init(&cmd_done, 1, 1);

    multicore_launch_core1(lighting_task);
}

void lighting_dispatch(lcmd_t const *cmd) {
    sem_acquire_blocking(&cmd_done);
    queue_add_blocking(&cmd_queue, cmd);
}

void lighting_wait(void) {
    sem_acquire_blocking(&cmd_done);
    sem_release(&cmd_done);
}
