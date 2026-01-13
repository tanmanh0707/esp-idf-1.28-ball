#pragma once
#include "config_app.h"

#define millis() (esp_timer_get_time() / 1000ULL)

void *ps_malloc(size_t size);
void *ps_calloc(size_t n, size_t size);
