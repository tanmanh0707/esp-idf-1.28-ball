#include "ntpclient.h"
#include "wifi.h"
#include "log_app.h"
#include <esp_netif_sntp.h>
#include <time.h>
#include <sys/time.h>

#define TAG "NTP"

SemaphoreHandle_t  NTPClient::_network_lock = nullptr;
EventGroupHandle_t NTPClient::_synced_event = nullptr;

#define NTP_SYNCED_BIT  BIT0

NTPClient& NTPClient::GetInstance() {
    static NTPClient instance;
    return instance;
}

NTPClient::NTPClient() : _initialized(false), _status(NTP_STATUS_IDLE) {}

void NTPClient::time_sync_notification_cb(struct timeval* tv) {
    log_i("NTP time synchronized");
    GetInstance()._status = NTP_STATUS_SYNCED;
    if (_synced_event) xEventGroupSetBits(_synced_event, NTP_SYNCED_BIT);
}

void NTPClient::InitSNTP() {
    setenv("TZ", CONFIG_NTP_TIMEZONE, 1);
    tzset();

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_NTP_SERVER);
    config.sync_cb = time_sync_notification_cb;

    while (_status != NTP_STATUS_SYNCED) {
        if (!TakeNetworkLock(0)) {
            log_w("NTP: network busy, retry in 10s");
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        esp_netif_sntp_deinit();
        esp_netif_sntp_init(&config);

        // Release immediately — UDP packet is in flight; callback fires asynchronously.
        GiveNetworkLock();
        log_i("NTP sync from %s...", CONFIG_NTP_SERVER);

        for (int i = 0; i < 120 && _status != NTP_STATUS_SYNCED; i++) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (_status == NTP_STATUS_SYNCED) break;

        log_w("NTP timeout, retry in 10s");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    char buf[32];
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    log_i("NTP sync complete: %s", buf);
}

void NTPClient::NTPTask(void* arg) {
    NTPClient* client = static_cast<NTPClient*>(arg);

    if (WiFiManager::WaitForConnection(portMAX_DELAY)) {
        client->_status = NTP_STATUS_NTP_SYNCING;
        client->InitSNTP();
    } else {
        log_w("NTP: WiFi unavailable, clock will not sync");
        client->_status = NTP_STATUS_FAILED;
    }
    vTaskDelete(nullptr);
}

void NTPClient::Init() {
    if (_initialized) return;
    _initialized  = true;
    _network_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(_network_lock);
    _synced_event = xEventGroupCreate();
    _status = NTP_STATUS_WIFI_CONNECTING;
    xTaskCreate(NTPTask, "ntp_task", 8192, this, 5, nullptr);
}

bool NTPClient::TakeNetworkLock(uint32_t timeout_ms) {
    if (!_network_lock) return true;
    return xSemaphoreTake(_network_lock, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void NTPClient::GiveNetworkLock() {
    if (_network_lock) xSemaphoreGive(_network_lock);
}

bool NTPClient::WaitForWifi(uint32_t timeout_ms) {
    return WiFiManager::WaitForConnection(timeout_ms);
}

bool NTPClient::WaitForSync(uint32_t timeout_ms) {
    if (!_synced_event) return false;
    TickType_t ticks = (timeout_ms == portMAX_DELAY)
                     ? portMAX_DELAY
                     : pdMS_TO_TICKS(timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(_synced_event, NTP_SYNCED_BIT,
                                            pdFALSE, pdTRUE, ticks);
    return (bits & NTP_SYNCED_BIT) != 0;
}

bool NTPClient::GetTime(struct tm* timeinfo) {
    if (!timeinfo) return false;
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    return true;
}
