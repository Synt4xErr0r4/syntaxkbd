#pragma once
#include <stdint.h>

#define TICKS_US(t) (t)
#define TICKS_MS(t) (1000 * (t))
#define TICKS_SEC(t) (1000000 * (t))

void ticks_init(void);
uint64_t ticks_since_start(void);
