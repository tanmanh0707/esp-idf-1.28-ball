#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <driver/i2s_std.h>
#include <driver/i2s_common.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstring>

#include "config_pins.h"
#include "log_app.h"
#include "system_info.h"

#define CONFIG_SDCARD_MOUNT_POINT     "/sd"
#define CONFIG_SDCARD_MOUNT_RETRY     3

/* Display */
#define CONFIG_DISPLAY_WIDTH                    240
#define CONFIG_DISPLAY_HEIGHT                   240
#define CONFIG_DISPLAY_MIRROR_X                 true
#define CONFIG_DISPLAY_MIRROR_Y                 false
#define CONFIG_DISPLAY_SWAP_XY                  false

#define CONFIG_DISPLAY_OFFSET_X                 0
#define CONFIG_DISPLAY_OFFSET_Y                 0
#define CONFIG_DISPLAY_BACKLIGHT_OUTPUT_INVERT  true

/* Codec */
#define AUDIO_INPUT_SAMPLE_RATE                 16000
#define AUDIO_OUTPUT_SAMPLE_RATE                16000//24000
#define AUDIO_CODEC_ES8311_ADDR                 ES8311_CODEC_DEFAULT_ADDR

/* WiFi Configuration */
#define CONFIG_WIFI_SSID                    "blackdragon_2.4"
#define CONFIG_WIFI_PASSWORD                "07071992"
#define CONFIG_WIFI_MAX_RETRY               5
#define CONFIG_WIFI_CONNECT_TIMEOUT_MS      15000

/* WiFi AP (config portal) */
#define CONFIG_WIFI_AP_SSID                 "ESP32-Setup"
#define CONFIG_WIFI_AP_CHANNEL              1
#define CONFIG_WIFI_AP_MAX_CONN             4

/* NTP Configuration */
#define CONFIG_NTP_SERVER                   "pool.ntp.org"
#define CONFIG_NTP_TIMEZONE                 "UTC-7"

/* HomeScreen Configuration */
#define CONFIG_HOMESCREEN_FOLDER            "/images"
#define CONFIG_HOMESCREEN_IMAGE_INTERVAL_MS 60000

/* Incoming Events Configuration */
#define CONFIG_EVENTS_URL \
    "https://script.google.com/macros/s/AKfycbwVKQzgvdtpKvMqds2vDujSFo5MMN7jmiI0WDkYK4H2rnKmT9IMIcLo5OFGZ73hoEP7ZA/exec?device=esp32-001&key=123456789"
#define CONFIG_EVENTS_POLL_INTERVAL_MS      (60 * 60 * 1000)   /* 1 hour  — on success */
#define CONFIG_EVENTS_RETRY_INTERVAL_MS     ( 5 * 60 * 1000)   /* 5 minutes — on failure */

/* Application */
#define BLOCKING                      true
#define NONE_BLOCKING                 false
#define MEMCMP_EQUAL                  0
#define STRCMP_EQUAL                  0
#define QUEUE_COPY_DATA               true
#define QUEUE_NO_COPY_DATA            false
#define QUEUE_SEND_TO_FRONT           true
