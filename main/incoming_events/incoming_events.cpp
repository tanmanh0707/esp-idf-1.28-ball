#include "incoming_events.h"
#include "ntpclient.h"
#include "log_app.h"
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define TAG "EVT"

#define GCAL_NVS_NAMESPACE  "gcal_cfg"
#define GCAL_NVS_KEY_URL    "url"
#define GCAL_NVS_KEY_DEVICE "device"
#define GCAL_NVS_KEY_KEY    "key"
#define GCAL_NVS_KEY_DAYS   "days"

static constexpr int RESPONSE_BUF_SIZE = 8192;

// ---------------------------------------------------------------------------
// HTTP context (passed via user_data)
// ---------------------------------------------------------------------------
struct HttpCtx {
    char* buf;
    int   len;
    char  location[512];   // captured from Location header on redirects
};

// Only used to capture the Location header for manual redirect following.
// Body data is read directly via esp_http_client_read() — no ON_DATA needed.
static esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    auto* ctx = static_cast<HttpCtx*>(evt->user_data);
    if (evt->event_id == HTTP_EVENT_ON_HEADER &&
        strcasecmp(evt->header_key, "location") == 0 && evt->header_value) {
        strncpy(ctx->location, evt->header_value, sizeof(ctx->location) - 1);
        ctx->location[sizeof(ctx->location) - 1] = '\0';
    }
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
IncomingEvents& IncomingEvents::GetInstance() {
    static IncomingEvents instance;
    return instance;
}

IncomingEvents::IncomingEvents()
    : _mutex(xSemaphoreCreateMutex()), _initialized(false),
      _version(0), _tasks_version(0) {
}

IncomingEvents::~IncomingEvents() {
    if (_mutex) vSemaphoreDelete(_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void IncomingEvents::Init() {
    if (_initialized) return;
    _initialized = true;
    xTaskCreate(EventTask, "events_task", 16384, this, 4, nullptr);
}

std::vector<IncomingEvent_t> IncomingEvents::GetEvents() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    auto copy = _events;
    xSemaphoreGive(_mutex);
    return copy;
}

std::vector<IncomingTask_t> IncomingEvents::GetTasks() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    auto copy = _tasks;
    xSemaphoreGive(_mutex);
    return copy;
}

// ---------------------------------------------------------------------------
// NVS config
// ---------------------------------------------------------------------------
bool IncomingEvents::LoadConfig(GCalConfig_t& cfg) {
    memset(&cfg, 0, sizeof(cfg));
    nvs_handle_t h;
    if (nvs_open(GCAL_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;

    size_t url_len    = sizeof(cfg.url);
    size_t device_len = sizeof(cfg.device);
    size_t key_len    = sizeof(cfg.key);
    size_t days_len   = sizeof(cfg.days);

    bool ok = (nvs_get_str(h, GCAL_NVS_KEY_URL,    cfg.url,    &url_len)    == ESP_OK) &&
              (nvs_get_str(h, GCAL_NVS_KEY_DEVICE,  cfg.device, &device_len) == ESP_OK) &&
              (nvs_get_str(h, GCAL_NVS_KEY_KEY,     cfg.key,    &key_len)    == ESP_OK) &&
              (nvs_get_str(h, GCAL_NVS_KEY_DAYS,    cfg.days,   &days_len)   == ESP_OK);
    nvs_close(h);

    if (!ok) return false;

    // Validate: non-empty strings and days is a number in [1, 90]
    if (cfg.url[0] == '\0' || cfg.device[0] == '\0' ||
        cfg.key[0] == '\0' || cfg.days[0] == '\0') return false;

    int days = atoi(cfg.days);
    if (days < 1 || days > 90) return false;

    return true;
}

void IncomingEvents::SaveConfig(const GCalConfig_t& cfg) {
    nvs_handle_t h;
    if (nvs_open(GCAL_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        log_e("GCal: failed to open NVS for write");
        return;
    }
    nvs_set_str(h, GCAL_NVS_KEY_URL,    cfg.url);
    nvs_set_str(h, GCAL_NVS_KEY_DEVICE, cfg.device);
    nvs_set_str(h, GCAL_NVS_KEY_KEY,    cfg.key);
    nvs_set_str(h, GCAL_NVS_KEY_DAYS,   cfg.days);
    nvs_commit(h);
    nvs_close(h);
    log_i("GCal config saved");
}

// ---------------------------------------------------------------------------
// GTask NVS config
// ---------------------------------------------------------------------------
bool IncomingEvents::LoadGTaskConfig(GTaskConfig_t& cfg) {
    memset(&cfg, 0, sizeof(cfg));
    nvs_handle_t h;
    if (nvs_open(CONFIG_GTASK_NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t url_len   = sizeof(cfg.url);
    size_t token_len = sizeof(cfg.token);
    bool ok = (nvs_get_str(h, CONFIG_GTASK_NVS_URL,   cfg.url,   &url_len)   == ESP_OK) &&
              (nvs_get_str(h, CONFIG_GTASK_NVS_TOKEN,  cfg.token, &token_len) == ESP_OK);
    nvs_close(h);
    if (!ok) return false;
    return cfg.url[0] != '\0' && cfg.token[0] != '\0';
}

void IncomingEvents::SaveGTaskConfig(const GTaskConfig_t& cfg) {
    nvs_handle_t h;
    if (nvs_open(CONFIG_GTASK_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        log_e("GTask: failed to open NVS for write");
        return;
    }
    nvs_set_str(h, CONFIG_GTASK_NVS_URL,   cfg.url);
    nvs_set_str(h, CONFIG_GTASK_NVS_TOKEN, cfg.token);
    nvs_commit(h);
    nvs_close(h);
    log_i("GTask config saved");
}

// ---------------------------------------------------------------------------
// Background task
// ---------------------------------------------------------------------------
void IncomingEvents::EventTask(void* arg) {
    auto* self = static_cast<IncomingEvents*>(arg);

    // Load and validate Google Calendar config from NVS
    GCalConfig_t gcal = {};
    if (!LoadConfig(gcal)) {
        log_w("Events: GCal config missing or invalid — fetching disabled");
        vTaskDelete(nullptr);
        return;
    }

    // Build full URL: base?device=X&key=Y&days=Z
    char full_url[512];
    snprintf(full_url, sizeof(full_url), "%s?device=%s&key=%s&days=%s",
             gcal.url, gcal.device, gcal.key, gcal.days);
    log_i("Events URL: %s", full_url);

    // Block until NTP sync completes — guarantees time is correct before
    // the first fetch and prevents NTP/HTTPS network contention at startup.
    if (!NTPClient::WaitForSync(portMAX_DELAY)) {
        log_w("Events: NTP sync never completed");
        vTaskDelete(nullptr);
        return;
    }

    log_i("Events: NTP synced, starting fetch loop");

    // Google Tasks config (optional — ok to be absent)
    GTaskConfig_t gtask = {};
    bool has_gtask = LoadGTaskConfig(gtask);
    if (has_gtask) {
        log_i("GTask URL: %s", gtask.url);
    } else {
        log_i("GTask config missing or invalid — task fetching disabled");
    }

    while (true) {
        if (!NTPClient::TakeNetworkLock(0)) {
            log_w("Events: network busy, retry in 10s");
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        bool ok = self->FetchAndParse(full_url);

        // Fetch tasks immediately after events, while lock is still held
        if (has_gtask) {
            self->FetchAndParseTasks(gtask.url, gtask.token);
        }

        NTPClient::GiveNetworkLock();

        uint32_t delay = ok ? CONFIG_EVENTS_POLL_INTERVAL_MS : CONFIG_EVENTS_RETRY_INTERVAL_MS;
        log_i("Next events fetch in %lu s", (unsigned long)(delay / 1000));
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

// ---------------------------------------------------------------------------
// HTTP fetch — manual redirect following
//
// esp_http_client_perform() reads the 302 response body before following the
// redirect. Google Apps Script sends that body as chunked transfer-encoding
// and closes the connection immediately, causing "Incomplete chunked data".
// Fix: use open()+fetch_headers() to read ONLY headers, never touch the body
// of a redirect response, then reissue to the Location URL manually.
// ---------------------------------------------------------------------------
bool IncomingEvents::FetchAndParse(const char* start_url) {
    char* resp = static_cast<char*>(
        heap_caps_malloc(RESPONSE_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!resp) {
        log_e("Failed to allocate response buffer");
        return false;
    }

    const char* url = start_url;
    char location_buf[512] = {};
    int  resp_len = 0;
    int  status   = 0;
    bool got_200  = false;

    for (int hop = 0; hop <= 5; hop++) {
        HttpCtx ctx = {};
        ctx.buf = resp;

        esp_http_client_config_t cfg = {};
        cfg.url                   = url;
        cfg.crt_bundle_attach     = esp_crt_bundle_attach;
        cfg.event_handler         = http_event_handler;
        cfg.user_data             = &ctx;
        cfg.timeout_ms            = 15000;
        cfg.method                = HTTP_METHOD_GET;
        cfg.max_redirection_count = 0;   // handled manually below

        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (!client) { log_e("HTTP client init failed"); break; }

        // Open connection and read ONLY headers — the body of a redirect
        // response is never touched, so chunked-encoding errors can't occur.
        if (esp_http_client_open(client, 0) != ESP_OK) {
            log_e("HTTP open failed");
            esp_http_client_cleanup(client);
            break;
        }
        esp_http_client_fetch_headers(client);
        status = esp_http_client_get_status_code(client);

        if (status == 301 || status == 302 || status == 303 ||
            status == 307 || status == 308) {
            // Location was captured by http_event_handler via ON_HEADER
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            if (ctx.location[0] == '\0') {
                log_e("Redirect without Location header");
                break;
            }
            strncpy(location_buf, ctx.location, sizeof(location_buf) - 1);
            url = location_buf;
            log_i("Redirect (%d) -> %s", status, url);
            continue;
        }

        if (status == 200) {
            int n;
            while (resp_len < RESPONSE_BUF_SIZE - 1) {
                n = esp_http_client_read(client, resp + resp_len,
                                          RESPONSE_BUF_SIZE - resp_len - 1);
                if (n <= 0) break;
                resp_len += n;
            }
            resp[resp_len] = '\0';
            got_200 = true;
        } else {
            log_e("HTTP GET failed: status=%d", status);
        }

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        break;
    }

    bool result = false;
    if (got_200) {
        log_i("Events response %d bytes", resp_len);
        result = ParseResponse(resp, resp_len);
    }

    heap_caps_free(resp);
    return result;
}

// ---------------------------------------------------------------------------
// HTTP fetch for Google Tasks — POST {"token":"..."} on first hop,
// follow any 3xx redirect as GET (RFC standard for POST redirects).
// ---------------------------------------------------------------------------
bool IncomingEvents::FetchAndParseTasks(const char* start_url, const char* token) {
    // Build the JSON payload once; token is bounded to 128 chars, no JSON-special chars expected
    char post_body[160];
    int  post_body_len = snprintf(post_body, sizeof(post_body), "{\"token\":\"%s\"}", token);

    char* resp = static_cast<char*>(
        heap_caps_malloc(RESPONSE_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!resp) { log_e("GTask: alloc failed"); return false; }

    const char*              url    = start_url;
    char                     loc[512] = {};
    esp_http_client_method_t method = HTTP_METHOD_POST;
    int  resp_len = 0;
    int  status   = 0;
    bool got_200  = false;

    for (int hop = 0; hop <= 5; hop++) {
        HttpCtx ctx = {};
        ctx.buf = resp;

        esp_http_client_config_t cfg = {};
        cfg.url                   = url;
        cfg.crt_bundle_attach     = esp_crt_bundle_attach;
        cfg.event_handler         = http_event_handler;
        cfg.user_data             = &ctx;
        cfg.timeout_ms            = 15000;
        cfg.method                = method;
        cfg.max_redirection_count = 0;

        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (!client) { log_e("GTask: client init failed"); break; }

        bool is_post = (method == HTTP_METHOD_POST);
        if (is_post) {
            esp_http_client_set_header(client, "Content-Type", "application/json");
        }

        // Open: pass body length so Content-Length header is set correctly on POST
        int write_len = is_post ? post_body_len : 0;
        if (esp_http_client_open(client, write_len) != ESP_OK) {
            log_e("GTask: open failed");
            esp_http_client_cleanup(client);
            break;
        }

        if (is_post) {
            esp_http_client_write(client, post_body, post_body_len);
        }

        esp_http_client_fetch_headers(client);
        status = esp_http_client_get_status_code(client);

        if (status == 301 || status == 302 || status == 303 ||
            status == 307 || status == 308) {
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            if (ctx.location[0] == '\0') { log_e("GTask: redirect without Location"); break; }
            strncpy(loc, ctx.location, sizeof(loc) - 1);
            url    = loc;
            method = HTTP_METHOD_GET;   // POST redirect followed as GET (RFC 7231 §6.4)
            log_i("GTask redirect (%d) -> %s", status, url);
            continue;
        }

        if (status == 200) {
            int n;
            while (resp_len < RESPONSE_BUF_SIZE - 1) {
                n = esp_http_client_read(client, resp + resp_len,
                                          RESPONSE_BUF_SIZE - resp_len - 1);
                if (n <= 0) break;
                resp_len += n;
            }
            resp[resp_len] = '\0';
            got_200 = true;
        } else {
            log_e("GTask: HTTP status=%d", status);
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        break;
    }

    bool result = false;
    if (got_200) {
        log_i("GTask response %d bytes", resp_len);
        result = ParseTasksResponse(resp, resp_len);
    }
    heap_caps_free(resp);
    return result;
}

bool IncomingEvents::ParseTasksResponse(const char* json, int len) {
    cJSON* root = cJSON_ParseWithLength(json, len);
    if (!root) { log_e("GTask: JSON parse error"); return false; }

    cJSON* arr = cJSON_GetObjectItemCaseSensitive(root, "tasks");
    if (!cJSON_IsArray(arr)) {
        log_w("GTask: expected object with 'tasks' array");
        cJSON_Delete(root);
        return false;
    }

    std::vector<IncomingTask_t> new_tasks;
    int count = cJSON_GetArraySize(arr);
    for (int i = 0; i < count; i++) {
        cJSON* item  = cJSON_GetArrayItem(arr, i);
        cJSON* title = cJSON_GetObjectItemCaseSensitive(item, "title");
        if (cJSON_IsString(title) && title->valuestring && title->valuestring[0] != '\0') {
            IncomingTask_t t;
            t.title = title->valuestring;
            new_tasks.push_back(std::move(t));
        }
    }

    log_i("Parsed %d task(s)", (int)new_tasks.size());

    xSemaphoreTake(_mutex, portMAX_DELAY);
    _tasks = std::move(new_tasks);
    _tasks_version++;
    xSemaphoreGive(_mutex);

    cJSON_Delete(root);
    return true;
}

// ---------------------------------------------------------------------------
// JSON parsing (calendar events)
// ---------------------------------------------------------------------------
bool IncomingEvents::ParseResponse(const char* json, int len) {
    cJSON* root = cJSON_ParseWithLength(json, len);
    if (!root) {
        log_e("JSON parse error");
        return false;
    }

    cJSON* success = cJSON_GetObjectItemCaseSensitive(root, "success");
    if (!cJSON_IsTrue(success)) {
        log_w("Events API: success=false");
        cJSON_Delete(root);
        return false;
    }

    cJSON* arr = cJSON_GetObjectItemCaseSensitive(root, "events");
    if (!cJSON_IsArray(arr)) {
        log_w("Events API: no events array");
        cJSON_Delete(root);
        return false;
    }

    std::vector<IncomingEvent_t> new_events;
    int count = cJSON_GetArraySize(arr);
    for (int i = 0; i < count && (int)new_events.size() < 3; i++) {
        cJSON* ev = cJSON_GetArrayItem(arr, i);
        cJSON* title    = cJSON_GetObjectItemCaseSensitive(ev, "title");
        cJSON* start    = cJSON_GetObjectItemCaseSensitive(ev, "start");
        cJSON* end      = cJSON_GetObjectItemCaseSensitive(ev, "end");
        cJSON* location = cJSON_GetObjectItemCaseSensitive(ev, "location");

        if (cJSON_IsString(title) && title->valuestring) {
            IncomingEvent_t e;
            e.title    = title->valuestring;
            if (cJSON_IsString(start)    && start->valuestring)    e.start    = start->valuestring;
            if (cJSON_IsString(end)      && end->valuestring)      e.end      = end->valuestring;
            if (cJSON_IsString(location) && location->valuestring) e.location = location->valuestring;
            new_events.push_back(std::move(e));
        }
    }

    log_i("Parsed %d event(s)", (int)new_events.size());
    for (auto& e : new_events) log_i("  • %s", e.title.c_str());

    xSemaphoreTake(_mutex, portMAX_DELAY);
    _events = std::move(new_events);
    _version++;
    xSemaphoreGive(_mutex);

    cJSON_Delete(root);
    return true;
}
