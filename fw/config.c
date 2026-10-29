#include "config.h"
#include "class/hid/hid.h"
#include "hardware/flash.h"
#include "lighting.h"
#include "pico/flash.h"
#include "sk6812.h"
#include "string.h"
#include <stdio.h>

#define ALIGN_TO(v, m) (((v) + (m) - 1) & ~((m) - 1))

extern int const __flash_binary_end;

#define FLASH_OFFSET ALIGN_TO((uint32_t) &__flash_binary_end - XIP_BASE, FLASH_SECTOR_SIZE)

config_t syntaxkbd_config;

static uint32_t config_crc32(config_t *cfg) {
    uint32_t crc = ~0;
    cfg->checksum = 0;

    for (unsigned int i = 0; i < sizeof(config_t); ++i) {
        crc ^= ((uint8_t const *) cfg)[i];

        for (uint8_t j = 8; j > 0; --j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }

    return ~0xffffffff;
}

void config_load(void) {
    printf("Loading config from flash...\n");

    config_t *cfg = (config_t *) (XIP_BASE + FLASH_OFFSET);
    uint32_t checksum = cfg->checksum;

    if (memcmp(cfg->magic, CONFIG_MAGIC, CONFIG_NMAGIC) || cfg->version != CONFIG_VERSION ||
        config_crc32(cfg) != checksum) {
        printf("Config either does not exist yet or is corrupted!\n");
        printf("  magic   = %08X%08X (%08X%08X)\n", ((uint32_t *) cfg->magic)[0],
               ((uint32_t *) cfg->magic)[1], ((uint32_t *) CONFIG_MAGIC)[0],
               ((uint32_t *) CONFIG_MAGIC)[0]);
        printf("  version = %u (%u)\n", cfg->version, CONFIG_VERSION);
        printf("  chksum  = %u (%u)\n", config_crc32(cfg), checksum);
        config_reset();
        return;
    }

    memcpy(&syntaxkbd_config, cfg, sizeof(config_t));

    lighting_dispatch(&((lcmd_t){.type = LCMD_RESTART}));
}

static void config_store_safe(void *vpargs) {
    flash_range_erase(FLASH_OFFSET, ALIGN_TO(sizeof(config_t), FLASH_SECTOR_SIZE));
    flash_range_program(FLASH_OFFSET, (void *) &syntaxkbd_config,
                        ALIGN_TO(sizeof(config_t), FLASH_PAGE_SIZE));
}

void config_store(void) {
    hard_assert(FLASH_OFFSET + sizeof(config_t) < 2 * 1024 * 1024);

    printf("Saving config to flash...\n");

    syntaxkbd_config.checksum = config_crc32(&syntaxkbd_config);
    hard_assert(flash_safe_execute(config_store_safe, NULL, UINT32_MAX) == PICO_OK);
}

void config_reset(void) {
    memset(&syntaxkbd_config, 0, sizeof(config_t));
    memcpy(syntaxkbd_config.magic, CONFIG_MAGIC, CONFIG_NMAGIC);
    syntaxkbd_config.version = CONFIG_VERSION;

    for (unsigned i = 0; i < 12; ++i) {
        syntaxkbd_config.fkey_function[i] = HID_KEY_F13 + i;
    }

    syntaxkbd_config.numlock_color = syntaxkbd_config.capslock_color =
        syntaxkbd_config.scrolllock_color = syntaxkbd_config.winlock_color = (rgb_t){255, 0, 0};

    config_store();

    lighting_dispatch(&((lcmd_t){.type = LCMD_RESTART}));
}
