#pragma once
#include "config_app.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_http_server.h>
#include <esp_event.h>

typedef enum {
    WIFI_STATE_IDLE        = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_AP_MODE,
    WIFI_STATE_FAILED,
} WiFiState_e;

class WiFiManager {
public:
    static WiFiManager& GetInstance();

    void        Init();
    WiFiState_e GetState()    const { return _state; }
    bool        IsConnected() const { return _state == WIFI_STATE_CONNECTED; }
    const char* GetIP()       const { return _ip_str; }

    // Block until STA has an IP (or timeout). Returns false on timeout / AP-only mode.
    static bool WaitForConnection(uint32_t timeout_ms = portMAX_DELAY);

    void SaveCredentials(const char* ssid, const char* pass);

private:
    WiFiManager();
    WiFiManager(const WiFiManager&)            = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;

    bool LoadCredentials(char* ssid, size_t ssid_len, char* pass, size_t pass_len);

    void StartAPMode();
    void StartWebServer();

    static void WiFiTask(void* arg);
    static void event_handler(void* arg, esp_event_base_t base,
                              int32_t id, void* event_data);

    volatile WiFiState_e _state;
    bool                 _initialized;
    int                  _retry_count;
    char                 _ip_str[16];
    httpd_handle_t       _server;

    static EventGroupHandle_t _connected_event;   // BIT0 set when STA gets IP
};
