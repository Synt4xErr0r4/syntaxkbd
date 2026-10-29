#include "time.h"
#include <hardware/timer.h>
#include <hardware/watchdog.h>

static uint64_t start;

void ticks_init(void) {
    start = time_us_64();
}

uint64_t ticks_since_start(void) {
    return time_us_64() - start;
}
