#include "wifi.h"
#include "wifi_html.h"
#include "log_app.h"
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_mac.h>
#include <cJSON.h>
#include <freertos/task.h>
#include <cstring>

#define TAG "WIFI"

#define WIFI_CONNECTED_BIT  BIT0

#define NVS_NAMESPACE   "wifi_cfg"
#define NVS_KEY_SSID    "ssid"
#define NVS_KEY_PASS    "pass"

// ── Statics ───────────────────────────────────────────────────────────────────
EventGroupHandle_t WiFiManager::_connected_event = nullptr;

// ── Singleton ─────────────────────────────────────────────────────────────────
WiFiManager& WiFiManager::GetInstance() {
    static WiFiManager instance;
    return instance;
}

WiFiManager::WiFiManager()
    : _state(WIFI_STATE_IDLE), _initialized(false), _retry_count(0), _server(nullptr) {
    _ip_str[0] = '\0';
}

// ── Init ──────────────────────────────────────────────────────────────────────
void WiFiManager::Init() {
    if (_initialized) return;
    _initialized    = true;
    _connected_event = xEventGroupCreate();
    xTaskCreate(WiFiTask, "wifi_task", 8192, this, 5, nullptr);
}

// ── WaitForConnection ─────────────────────────────────────────────────────────
bool WiFiManager::WaitForConnection(uint32_t timeout_ms) {
    if (!_connected_event) return false;
    TickType_t ticks = (timeout_ms == portMAX_DELAY)
                     ? portMAX_DELAY
                     : pdMS_TO_TICKS(timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(_connected_event, WIFI_CONNECTED_BIT,
                                            pdFALSE, pdTRUE, ticks);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

// ── NVS credentials ───────────────────────────────────────────────────────────
bool WiFiManager::LoadCredentials(char* ssid, size_t ssid_len, char* pass, size_t pass_len) {
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
    bool ok = (nvs_get_str(h, NVS_KEY_SSID, ssid, &ssid_len) == ESP_OK) &&
              (ssid[0] != '\0');
    if (ok) nvs_get_str(h, NVS_KEY_PASS, pass, &pass_len);
    nvs_close(h);
    return ok;
}

void WiFiManager::SaveCredentials(const char* ssid, const char* pass) {
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        log_e("Failed to open NVS for write");
        return;
    }
    nvs_set_str(h, NVS_KEY_SSID, ssid);
    nvs_set_str(h, NVS_KEY_PASS, pass ? pass : "");
    nvs_commit(h);
    nvs_close(h);
    log_i("Credentials saved: ssid=%s", ssid);
}

// ── WiFi event handler ────────────────────────────────────────────────────────
void WiFiManager::event_handler(void* arg, esp_event_base_t base,
                                 int32_t id, void* event_data) {
    WiFiManager* self = static_cast<WiFiManager*>(arg);

    // Ignore STA events when running in AP-only config mode
    if (self->_state == WIFI_STATE_AP_MODE) return;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self->_state = WIFI_STATE_DISCONNECTED;
        if (self->_retry_count < CONFIG_WIFI_MAX_RETRY) {
            esp_wifi_connect();
            self->_retry_count++;
            log_i("WiFi retry %d/%d", self->_retry_count, CONFIG_WIFI_MAX_RETRY);
        } else {
            self->_state = WIFI_STATE_FAILED;
            log_w("WiFi failed after %d retries", CONFIG_WIFI_MAX_RETRY);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* e = static_cast<ip_event_got_ip_t*>(event_data);
        snprintf(self->_ip_str, sizeof(self->_ip_str), IPSTR, IP2STR(&e->ip_info.ip));
        log_i("WiFi connected, IP: %s", self->_ip_str);
        self->_retry_count = 0;
        self->_state       = WIFI_STATE_CONNECTED;
        xEventGroupSetBits(_connected_event, WIFI_CONNECTED_BIT);
        self->StartWebServer();   // config portal available at http://<ip>
    }
}

// ── (StartSTAMode merged into WiFiTask — see below) ───────────────────────────

// ── HTTP handlers (forward declarations) ─────────────────────────────────────
static esp_err_t handle_root(httpd_req_t* req);
static esp_err_t handle_scan(httpd_req_t* req);
static esp_err_t handle_save(httpd_req_t* req);
static void      reboot_task(void* arg);

// ── AP mode + web server ──────────────────────────────────────────────────────
void WiFiManager::StartAPMode() {
    _state = WIFI_STATE_AP_MODE;
    log_i("Starting AP config mode: SSID=%s", CONFIG_WIFI_AP_SSID);

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // APSTA so we can scan for nearby networks from within the HTTP handler
    wifi_config_t ap_cfg = {};
    strncpy((char*)ap_cfg.ap.ssid, CONFIG_WIFI_AP_SSID, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid_len       = strlen(CONFIG_WIFI_AP_SSID);
    ap_cfg.ap.channel        = CONFIG_WIFI_AP_CHANNEL;
    ap_cfg.ap.authmode       = WIFI_AUTH_OPEN;
    ap_cfg.ap.max_connection = CONFIG_WIFI_AP_MAX_CONN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    StartWebServer();
}

void WiFiManager::StartWebServer() {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size     = 8192;

    if (httpd_start(&_server, &cfg) != ESP_OK) {
        log_e("Failed to start web server");
        return;
    }

    httpd_uri_t root = { "/",     HTTP_GET,  handle_root, this };
    httpd_uri_t scan = { "/scan", HTTP_GET,  handle_scan, this };
    httpd_uri_t save = { "/save", HTTP_POST, handle_save, this };

    httpd_register_uri_handler(_server, &root);
    httpd_register_uri_handler(_server, &scan);
    httpd_register_uri_handler(_server, &save);

    log_i("Config server ready — connect to '%s' then open http://192.168.4.1",
          CONFIG_WIFI_AP_SSID);
}

// ── WiFi task ─────────────────────────────────────────────────────────────────
void WiFiManager::WiFiTask(void* arg) {
    WiFiManager* self = static_cast<WiFiManager*>(arg);

    esp_netif_init();

    char ssid[64] = {};
    char pass[64] = {};

    if (!self->LoadCredentials(ssid, sizeof(ssid), pass, sizeof(pass))) {
        log_i("No WiFi credentials in NVS — AP config mode");
        self->StartAPMode();
        vTaskDelete(nullptr);
        return;
    }

    // ── Credentials found: scan to verify the network is reachable ─────────────
    log_i("Credentials found, scanning for '%s'...", ssid);

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());   // triggers STA_START (no handler yet — ignored)

    wifi_scan_config_t scan_cfg = {};
    scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    esp_wifi_scan_start(&scan_cfg, true);   // blocking

    uint16_t         ap_count = 32;
    wifi_ap_record_t records[32];
    esp_wifi_scan_get_ap_records(&ap_count, records);

    bool found = false;
    for (uint16_t i = 0; i < ap_count; i++) {
        if (strcmp((char*)records[i].ssid, ssid) == 0) { found = true; break; }
    }

    if (!found) {
        log_w("'%s' not found in scan — falling back to AP config mode", ssid);
        esp_wifi_stop();
        esp_wifi_deinit();
        self->StartAPMode();
        vTaskDelete(nullptr);
        return;
    }

    // ── SSID visible: register handlers and connect ────────────────────────────
    // WiFi is already started (scan used it); STA_START event is long past.
    // Register handlers now, then call connect directly.
    log_i("'%s' found — connecting", ssid);
    self->_state = WIFI_STATE_CONNECTING;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, self, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, self, nullptr));

    wifi_config_t wifi_cfg = {};
    strncpy((char*)wifi_cfg.sta.ssid,     ssid, sizeof(wifi_cfg.sta.ssid)     - 1);
    strncpy((char*)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_wifi_connect();

    vTaskDelete(nullptr);
}

// ── HTTP: settings page ───────────────────────────────────────────────────────
static esp_err_t handle_root(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, SETTINGS_HTML);
    return ESP_OK;
}

// ── HTTP: WiFi scan ───────────────────────────────────────────────────────────
static esp_err_t handle_scan(httpd_req_t* req) {
    wifi_scan_config_t scan_cfg = {};
    scan_cfg.scan_type          = WIFI_SCAN_TYPE_ACTIVE;
    esp_wifi_scan_start(&scan_cfg, true);   // blocking (runs in httpd task)

    uint16_t ap_count = 32;
    wifi_ap_record_t records[32];
    esp_wifi_scan_get_ap_records(&ap_count, records);

    cJSON* arr = cJSON_CreateArray();

    // Deduplicate by SSID; keep the strongest signal
    char seen[32][33] = {};
    int  seen_count   = 0;

    for (int i = 0; i < ap_count; i++) {
        const char* ssid = (char*)records[i].ssid;
        if (ssid[0] == '\0') continue;

        bool duplicate = false;
        for (int j = 0; j < seen_count; j++) {
            if (strcmp(seen[j], ssid) == 0) { duplicate = true; break; }
        }
        if (duplicate) continue;

        if (seen_count < 32) strncpy(seen[seen_count++], ssid, 32);

        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", ssid);
        cJSON_AddNumberToObject(item, "rssi", records[i].rssi);
        cJSON_AddBoolToObject(item, "auth", records[i].authmode != WIFI_AUTH_OPEN);
        cJSON_AddItemToArray(arr, item);
    }

    char* json = cJSON_PrintUnformatted(arr);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json ? json : "[]");
    if (json) cJSON_free(json);
    cJSON_Delete(arr);
    return ESP_OK;
}

// ── HTTP: save settings ───────────────────────────────────────────────────────
static void reboot_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

static esp_err_t handle_save(httpd_req_t* req) {
    char buf[512] = {};
    int  received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }
    buf[received] = '\0';

    httpd_resp_set_type(req, "application/json");

    cJSON* root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
        return ESP_OK;
    }

    cJSON* j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON* j_pass = cJSON_GetObjectItemCaseSensitive(root, "password");

    if (!cJSON_IsString(j_ssid) || !j_ssid->valuestring || j_ssid->valuestring[0] == '\0') {
        cJSON_Delete(root);
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"SSID is required\"}");
        return ESP_OK;
    }

    const char* ssid = j_ssid->valuestring;
    const char* pass = (cJSON_IsString(j_pass) && j_pass->valuestring) ? j_pass->valuestring : "";

    if (strlen(ssid) > 32) {
        cJSON_Delete(root);
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"SSID too long\"}");
        return ESP_OK;
    }

    WiFiManager* self = static_cast<WiFiManager*>(req->user_ctx);
    self->SaveCredentials(ssid, pass);
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "{\"ok\":true}");

    // Reboot after the response is flushed
    xTaskCreate(reboot_task, "reboot", 2048, nullptr, 1, nullptr);
    return ESP_OK;
}
