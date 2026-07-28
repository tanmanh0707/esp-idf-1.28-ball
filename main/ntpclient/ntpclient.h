#pragma once
#include "config_app.h"
#include <ctime>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

typedef enum {
    NTP_STATUS_IDLE            = 0,
    NTP_STATUS_WIFI_CONNECTING,
    NTP_STATUS_NTP_SYNCING,
    NTP_STATUS_SYNCED,
    NTP_STATUS_FAILED,
} NTPStatus_e;

class NTPClient {
public:
    static NTPClient& GetInstance();

    void        Init();
    NTPStatus_e GetStatus()    const { return _status; }
    bool        IsTimeSynced() const { return _status == NTP_STATUS_SYNCED; }
    bool        GetTime(struct tm* timeinfo);

    // Blocks until WiFi has an IP (delegates to WiFiManager::WaitForConnection).
    static bool WaitForWifi(uint32_t timeout_ms = portMAX_DELAY);

    // Blocks until NTP has completed its first successful sync (or timeout).
    static bool WaitForSync(uint32_t timeout_ms = portMAX_DELAY);

    // Network-access mutex shared with IncomingEvents.
    // Non-blocking (timeout_ms=0): returns false immediately if busy.
    static bool TakeNetworkLock(uint32_t timeout_ms = 0);
    static void GiveNetworkLock();

private:
    NTPClient();
    NTPClient(const NTPClient&) = delete;
    NTPClient& operator=(const NTPClient&) = delete;

    void InitSNTP();

    bool        _initialized;
    NTPStatus_e _status;

    static void NTPTask(void* arg);
    static void time_sync_notification_cb(struct timeval* tv);
    static SemaphoreHandle_t  _network_lock;  // binary semaphore shared with IncomingEvents
    static EventGroupHandle_t _synced_event;  // BIT0 set when first NTP sync completes
};
