#pragma once
#include "config.h"
#include "sk6812.h"
#include <pico/types.h>

typedef enum {
    LCMD_RESTART,
    LCMD_OFF,
    LCMD_HIGHLIGHT,
    LCMD_PREVIEW,
} lcmd_type_t;

typedef struct {
    uint col, row;
    rgb_t color;
} lcmd_highlight_t;

typedef struct {
    lcmd_type_t type;

    union {
        struct {
            lcmd_highlight_t *highlights;
            size_t nhighlights;
        } highlight;

        struct {
            effect_t effect;
        } preview;
    };

} lcmd_t;

void lighting_init(void);
void lighting_dispatch(lcmd_t const *);
void lighting_wait(void);
