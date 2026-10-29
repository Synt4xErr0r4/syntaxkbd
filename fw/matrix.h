#pragma once

#include "pico/types.h"
#include <stdint.h>

#define NCOL 22
#define NROW 6

#define NKEYARRAY 6
#define NMULTIMEDIA 1

typedef enum {
    KEY_FN_MASK = 0xF000,

    KEY_MISC_MASK = 0x1000,

    KEY_MACRO_MASK = 0x2000,
    KEY_MACRO_BITS = 0x0007,

    KEY_MULTIMEDIA_MASK = 0x3000,
    KEY_MULTIMEDIA_BITS = 0x03FF,

    // miscellaneous keys
    KEY_WINLOCK = KEY_MISC_MASK | 0x01,
    KEY_LIGHTING_TOGGLE = KEY_MISC_MASK | 0x02,
} fn_key_t;

typedef struct {
    uint8_t array[NKEYARRAY];
    uint8_t narray;
    bool rollover;

    uint8_t bitmap[32];
    uint8_t modifiers;

    uint16_t multimedia[NMULTIMEDIA];
    uint8_t nmultimedia;

    uint8_t macros;
} matrix_result_t;

void matrix_init(void);
void matrix_query(matrix_result_t *);
void matrix_result_reset(matrix_result_t *);
void matrix_result_add(matrix_result_t *, uint16_t);
uint16_t matrix_keycode(uint row, uint col, bool fn);

// #define SYNTAXKBD_DEBUG
#ifdef SYNTAXKBD_DEBUG
const char *matrix_keyname(uint16_t);
#endif
