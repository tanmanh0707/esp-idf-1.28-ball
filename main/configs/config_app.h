#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include <memory>
#include <string>

#include "config_pins.h"

/* Application */
#define BLOCKING                      true
#define NONE_BLOCKING                 false
#define MEMCMP_EQUAL                  0
#define STRCMP_EQUAL                  0
#define QUEUE_COPY_DATA               true
#define QUEUE_NO_COPY_DATA            false
#define QUEUE_SEND_TO_FRONT           true
