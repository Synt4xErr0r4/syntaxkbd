#pragma once
#include <stdint.h>

void led_init(void);
void led_recv_state(uint8_t state);
void led_refresh(void);
