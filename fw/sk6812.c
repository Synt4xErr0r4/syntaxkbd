#include "sk6812.h"
#include "hardware/dma.h"
#include "sk6812.pio.h"
#include <math.h>
#include <pico/time.h>
#include <string.h>

#define SK6812_FREQUENCY 800000.f

static const uint8_t GAMMA[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,
    4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,
    9,   10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,
    18,  18,  19,  19,  20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,
    29,  30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,
    43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
    61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  81,
    82,  83,  84,  85,  87,  88,  89,  90,  91,  93,  94,  95,  97,  98,  99,  100, 102, 103, 105,
    106, 107, 109, 110, 111, 113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132,
    133, 135, 137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161, 163,
    165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190, 192, 194, 196, 197,
    199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 234, 236,
    238, 240, 242, 244, 246, 248, 251, 253, 255,
};

static int s_off_pio0 = -1;
static int s_off_pio1 = -1;

static void sk6812_build_frame(sk6812_t *);
static void sk6812_start_dma(sk6812_t *);
static void sk6812_stop_dma(sk6812_t *);

bool sk6812_begin(sk6812_t *sk) {
    if (sk->pio == pio0) {
        if (s_off_pio0 < 0) {
            s_off_pio0 = pio_add_program(sk->pio, &sk6812_program);
        }
        sk->pio_off = (uint) s_off_pio0;
    } else {
        if (s_off_pio1 < 0) {
            s_off_pio1 = pio_add_program(sk->pio, &sk6812_program);
        }
        sk->pio_off = (uint) s_off_pio1;
    }

    if (sk->sm < 0) {
        sk->sm = pio_claim_unused_sm(sk->pio, true);
        if (sk->sm < 0) {
            return false;
        }
    }

    sk6812_program_init(sk->pio, (uint) sk->sm, sk->pio_off, sk->pin, SK6812_FREQUENCY);

    sk->dma_ch = dma_claim_unused_channel(false);
    if (sk->dma_ch >= 0) {
        dma_channel_config c = dma_channel_get_default_config(sk->dma_ch);
        channel_config_set_dreq(&c, pio_get_dreq(sk->pio, (uint) sk->sm, true));
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        dma_channel_configure(sk->dma_ch, &c, (volatile void *) &sk->pio->txf[sk->sm], NULL, 0,
                              false);
    }

    sk6812_clear(sk);
    return true;
}

void sk6812_end(sk6812_t *sk) {
    sk6812_stop_dma(sk);
    if (sk->dma_ch >= 0) {
        dma_channel_unclaim(sk->dma_ch);
        sk->dma_ch = -1;
    }
    if (sk->sm >= 0) {
        pio_sm_set_enabled(sk->pio, (uint) sk->sm, false);
        pio_sm_unclaim(sk->pio, (uint) sk->sm);
        sk->sm = -1;
    }
}

uint sk6812_size(sk6812_t *const sk) {
    return sk->count;
}

void sk6812_clear(sk6812_t *sk) {
    memset(sk->buf, 0, sk6812_size(sk) * sizeof(rgb_t));
}

void sk6812_set(sk6812_t *sk, uint i, rgb_t rgb) {
    if (i < sk->count) {
        sk->buf[i] = rgb;
    }
}

void sk6812_brightness(sk6812_t *sk, uint8_t brightness) {
    sk->brightness = brightness;
}

void sk6812_gamma(sk6812_t *sk, bool gamma) {
    sk->gamma = gamma;
}

void sk6812_show(sk6812_t *sk) {
    sk6812_show_async(sk);
    sk6812_wait(sk);
}

void sk6812_show_async(sk6812_t *sk) {
    sk6812_build_frame(sk);
    sk6812_start_dma(sk);
}

bool sk6812_busy(sk6812_t *const sk) {
    return sk->dma_ch >= 0 && dma_channel_is_busy(sk->dma_ch);
}

void sk6812_wait(sk6812_t *sk) {
    if (sk->dma_ch >= 0) {
        while (dma_channel_is_busy(sk->dma_ch)) {
            tight_loop_contents();
        }
    }

    sleep_us(80);
}

static inline uint8_t scale_u8(uint8_t v, uint8_t b) {
    return (uint8_t) (((uint16_t) v * (uint16_t) b + 127) / 255);
}

static void sk6812_build_frame(sk6812_t *sk) {
    for (uint i = 0; i < sk->count; ++i) {
        rgb_t p = sk->buf[i];
        uint8_t r = scale_u8(sk->gamma ? GAMMA[p.r] : p.r, sk->brightness);
        uint8_t g = scale_u8(sk->gamma ? GAMMA[p.g] : p.g, sk->brightness);
        uint8_t b = scale_u8(sk->gamma ? GAMMA[p.b] : p.b, sk->brightness);

        uint32_t grb = ((uint32_t) g << 16) | ((uint32_t) r << 8) | b;
        sk->tx_frame[i] = (grb << 8);
    }
}

static void sk6812_start_dma(sk6812_t *sk) {
    if (sk->dma_ch < 0) {
        for (size_t i = 0; i < sk->count; ++i) {
            pio_sm_put_blocking(sk->pio, (uint) sk->sm, sk->tx_frame[i]);
        }
        return;
    }

    dma_channel_set_read_addr(sk->dma_ch, sk->tx_frame, false);
    dma_channel_set_trans_count(sk->dma_ch, (uint) sk->count, true);
}

static void sk6812_stop_dma(sk6812_t *sk) {
    if (sk->dma_ch >= 0) {
        dma_channel_abort(sk->dma_ch);
    }
}

static float clamp(float v, float min, float max) {
    return MIN(MAX(v, min), max);
}

rgb_t hsv2rgb(float h, float s, float v) {
    h = fmodf(h, 360.0f);
    if (h < 0) {
        h += 360.0f;
    }
    s = clamp(s, 0.0f, 1.0f);
    v = clamp(v, 0.0f, 1.0f);
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    int seg = (int) (h / 60.0f);
    switch (seg) {
    case 0:
        r = c;
        g = x;
        b = 0;
        break;
    case 1:
        r = x;
        g = c;
        b = 0;
        break;
    case 2:
        r = 0;
        g = c;
        b = x;
        break;
    case 3:
        r = 0;
        g = x;
        b = c;
        break;
    case 4:
        r = x;
        g = 0;
        b = c;
        break;
    default:
        r = c;
        g = 0;
        b = x;
        break;
    }

    return (rgb_t){
        (uint8_t) roundf((r + m) * 255.0f),
        (uint8_t) roundf((g + m) * 255.0f),
        (uint8_t) roundf((b + m) * 255.0f),
    };
}
