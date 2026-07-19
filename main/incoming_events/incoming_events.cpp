#include "incoming_events.h"
#include "ntpclient.h"
#include "log_app.h"
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <cJSON.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define TAG "EVT"

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
    : _mutex(xSemaphoreCreateMutex()), _initialized(false), _version(0) {
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

// ---------------------------------------------------------------------------
// Background task
// ---------------------------------------------------------------------------
void IncomingEvents::EventTask(void* arg) {
    auto* self = static_cast<IncomingEvents*>(arg);

    // Block until WiFi has an IP — triggered by the same IP_EVENT_STA_GOT_IP
    // that the NTP task uses, so the first fetch happens the instant WiFi connects.
    if (!NTPClient::WaitForWifi(portMAX_DELAY)) {
        log_w("Events: WiFi never became ready");
        vTaskDelete(nullptr);
        return;
    }

    log_i("Events: WiFi ready, waiting for network lock");

    while (true) {
        // Non-blocking acquire — if NTP is currently syncing, back off 10 s.
        if (!NTPClient::TakeNetworkLock(0)) {
            log_w("Events: network busy, retry in 10s");
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        bool ok = self->FetchAndParse();
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
bool IncomingEvents::FetchAndParse() {
    char* resp = static_cast<char*>(
        heap_caps_malloc(RESPONSE_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!resp) {
        log_e("Failed to allocate response buffer");
        return false;
    }

    const char* url = CONFIG_EVENTS_URL;
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
// JSON parsing
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
