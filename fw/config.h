#pragma once
#include "sk6812.h"
#include <stdint.h>

#define CONFIG_NMAGIC 8
#define CONFIG_MAGIC "SyntxKBD"

#define CONFIG_VERSION 1

#define CONFIG_NMACRO 10
#define CONFIG_NMACRO_ACTION 1024

#define CONFIG_NEFFECT 10

typedef enum {
    EFFECT_TYPE_OFF,
    EFFECT_TYPE_STATIC,
    EFFECT_TYPE_RAINBOW,
    EFFECT_TYPE_WAVE,
    EFFECT_TYPE_WHITE,
} effect_type_t;

typedef enum {
    EFFECT_DIR_NORTH = 0b0010,
    EFFECT_DIR_SOUTH = 0b0011,
    EFFECT_DIR_WEST = 0b1000,
    EFFECT_DIR_EAST = 0b1100,
    EFFECT_DIR_NORTHEAST = EFFECT_DIR_NORTH | EFFECT_DIR_EAST,
    EFFECT_DIR_NORTHWEST = EFFECT_DIR_NORTH | EFFECT_DIR_WEST,
    EFFECT_DIR_SOUTHWEST = EFFECT_DIR_SOUTH | EFFECT_DIR_WEST,
    EFFECT_DIR_SOUTHEAST = EFFECT_DIR_SOUTH | EFFECT_DIR_EAST,
} effect_direction_t;

typedef struct __attribute__((packed)) {
    effect_type_t type : 7;
    uint8_t breathing : 1;
    float brightness;
    float hue;
    float speed;
    float breathing_speed;
    effect_direction_t direction;
} effect_t;

typedef enum {
    MACRO_ACTION_DONE,
    MACRO_ACTION_DELAY,
    MACRO_ACTION_KEYUP,
    MACRO_ACTION_KEYDOWN,
} macro_action_type_t;

typedef struct __attribute__((packed)) {
    macro_action_type_t type : 2;
    unsigned : 6;
    uint8_t value;
} macro_action_t;

typedef struct __attribute__((packed)) {
    macro_action_t actions[CONFIG_NMACRO_ACTION];
} macro_t;

typedef struct __attribute__((packed)) {
    char magic[CONFIG_NMAGIC];
    uint32_t version;

    union __attribute__((packed)) {
        uint8_t flags;
        struct __attribute__((packed)) {
            uint8_t flag_winlock : 1;
            uint8_t flag_lighting : 1;
            uint8_t : 6;
        };
    };

    macro_t macros[CONFIG_NMACRO];

    effect_t effects[CONFIG_NEFFECT];
    uint8_t selected_effect;

    uint16_t fkey_function[12];

    rgb_t numlock_color;
    rgb_t capslock_color;
    rgb_t scrolllock_color;
    rgb_t winlock_color;

    uint32_t checksum;
} config_t;

extern config_t syntaxkbd_config;

void config_load(void);
void config_store(void);
void config_reset(void);
