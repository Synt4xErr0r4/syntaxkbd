#pragma once
/// Vendor-specific protocol.
/// Used for configuration and information retrieval

#include "config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    PACKET_ACK = 0x00,
    PACKET_NAK = 0x01,

    PACKET_VERSION = 0x10,
    PACKET_FLAGS = 0x11,
    PACKET_EFFECTS = 0x12,
    PACKET_MACROS = 0x13,
    PACKET_FKEYS = 0x14,
    PACKET_COLORS = 0x15,

    PACKET_QUERY_MASK = 0x00,
    PACKET_SET_MASK = 0x80,
};

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t body[];
} packet_t;

typedef struct __attribute__((packed)) {
    uint32_t version;
    char name[32];
} packet_version_t;

typedef union __attribute__((packed)) {
    uint8_t flags;
    struct __attribute__((packed)) {
        uint8_t flag_winlock : 1;
        uint8_t flag_lighting : 1;
        uint8_t : 6;
    };
} packet_flags_t;

typedef struct __attribute__((packed)) {
    effect_t effects[CONFIG_NEFFECT];
    uint8_t selected_effect;
} packet_effects_t;

typedef struct __attribute__((packed)) {
    macro_t macros[CONFIG_NMACRO];
} packet_macros_t;

typedef struct __attribute__((packed)) {
    uint16_t fkey_function[12];
} packet_fkeys_t;

typedef struct __attribute__((packed)) {
    rgb_t numlock_color;
    rgb_t capslock_color;
    rgb_t scrolllock_color;
    rgb_t winlock_color;
} packet_colors_t;
