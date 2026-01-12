#pragma once
#include "esp_log.h"

#define log_e(...) ESP_LOGE(TAG, __VA_ARGS__)
#define log_w(...) ESP_LOGW(TAG, __VA_ARGS__)
#define log_i(...) ESP_LOGI(TAG, __VA_ARGS__)
#define log_d(...) ESP_LOGD(TAG, __VA_ARGS__)
#define log_v(...) ESP_LOGV(TAG, __VA_ARGS__)