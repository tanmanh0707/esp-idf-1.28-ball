#include "homescreen.h"
#include "ntpclient.h"
#include "wifi.h"
#include "incoming_events.h"
#include "sdcard.h"
#include "log_app.h"
#include "LexendRegular.h"
#include <esp_jpeg_dec.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "HOME"

// ── Bottom clock / status badge ──────────────────────────────────────────────
static constexpr int BADGE_X = 15;
static constexpr int BADGE_Y = 170;
static constexpr int BADGE_W = 210;
static constexpr int BADGE_H = 64;
static constexpr int BADGE_R = 12;

// ── Top events badge ─────────────────────────────────────────────────────────
static constexpr int EV_X   = 15;
static constexpr int EV_Y   = 8;
static constexpr int EV_W   = 210;
static constexpr int EV_R   = 10;
static constexpr int EV_ROW = 26;   // px per event row — sized for Lexend_Regular20
static constexpr int EV_PAD = 10;   // top + bottom internal padding

// ─────────────────────────────────────────────────────────────────────────────

HomeScreen::HomeScreen(lgfx::LGFX_Device* lcd, HomeScreenType_e type)
    : _lcd(lcd), _type(type), _bg_sprite(lcd), _clock_sprite(lcd) {
}

HomeScreen::~HomeScreen() {
    _clock_sprite.deleteSprite();
    _bg_sprite.deleteSprite();
}

void HomeScreen::Start() {
    switch (_type) {
        case HOMESCREEN_TYPE_DIGITAL_CLOCK_BG: RunDigitalClockBg(); break;
        default: log_e("Unknown HomeScreen type: %d", _type); break;
    }
}

// ─── JPEG → background sprite ────────────────────────────────────────────────
bool HomeScreen::LoadJpegToSprite(const std::string& filename) {
    std::string path = std::string(CONFIG_HOMESCREEN_FOLDER) + "/" + filename;
    log_i("Loading background: %s", path.c_str());

    uint8_t* jpeg_data = nullptr;
    size_t   file_size = 0;

    if (SDCARD_ReadFile(path.c_str(), &jpeg_data, file_size) != ESP_OK) {
        log_e("Failed to read: %s", path.c_str());
        return false;
    }

    uint16_t* pixels = nullptr;
    uint32_t  width = 0, height = 0;

    if (SDCARD_DecodeJpeg(jpeg_data, file_size, &pixels, &width, &height) != ESP_OK) {
        log_e("Failed to decode JPEG: %s", path.c_str());
        heap_caps_free(jpeg_data);
        return false;
    }
    heap_caps_free(jpeg_data);

    _bg_sprite.fillSprite(TFT_BLACK);
    int16_t  x      = ((int16_t)CONFIG_DISPLAY_WIDTH  - (int16_t)width)  / 2;
    int16_t  y      = ((int16_t)CONFIG_DISPLAY_HEIGHT - (int16_t)height) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    uint16_t disp_w = (width  > CONFIG_DISPLAY_WIDTH)  ? CONFIG_DISPLAY_WIDTH  : (uint16_t)width;
    uint16_t disp_h = (height > CONFIG_DISPLAY_HEIGHT) ? CONFIG_DISPLAY_HEIGHT : (uint16_t)height;

    _bg_sprite.startWrite();
    _bg_sprite.setAddrWindow(x, y, disp_w, disp_h);
    _bg_sprite.pushPixels(pixels, (uint32_t)disp_w * disp_h);
    _bg_sprite.endWrite();

    jpeg_free_align(pixels);
    return true;
}

// ─── Events badge (drawn onto clock_sprite) ───────────────────────────────────
void HomeScreen::DrawEventsBadge() {
    auto events = IncomingEvents::GetInstance().GetEvents();
    if (events.empty()) return;

    int n   = (int)events.size();
    if (n > 3) n = 3;
    int bh  = EV_PAD + n * EV_ROW + EV_PAD;

    _clock_sprite.fillRoundRect(EV_X, EV_Y, EV_W, bh, EV_R,
                                 _clock_sprite.color565(0, 0, 50));

    _clock_sprite.loadFont(Lexend_Regular20);
    _clock_sprite.setTextColor(TFT_WHITE);
    _clock_sprite.setTextDatum(lgfx::MC_DATUM);

    for (int i = 0; i < n; i++) {
        int text_y = EV_Y + EV_PAD + i * EV_ROW + EV_ROW / 2;
        _clock_sprite.drawString(events[i].title.c_str(), EV_X + EV_W / 2, text_y);
    }
    _clock_sprite.unloadFont();
}

// ─── Unified compositor ───────────────────────────────────────────────────────
// bottom_text == nullptr → no bottom badge (background + events only)
void HomeScreen::RenderFrame(const char*        bottom_text,
                              const lgfx::IFont* bottom_font,
                              uint16_t           bottom_color) {
    // 1. Blit clean background into compositing sprite
    _bg_sprite.pushSprite(&_clock_sprite, 0, 0);

    // 2. Overlay events badge at top (drawn with Lexend via loadFont/unloadFont)
    DrawEventsBadge();

    // 3. Overlay bottom badge (clock → DejaVu40, status → DejaVu18)
    if (bottom_text) {
        _clock_sprite.fillRoundRect(BADGE_X, BADGE_Y, BADGE_W, BADGE_H, BADGE_R,
                                     _clock_sprite.color565(10, 10, 10));
        _clock_sprite.setFont(bottom_font);
        _clock_sprite.setTextColor(bottom_color);
        _clock_sprite.setTextDatum(lgfx::MC_DATUM);
        _clock_sprite.drawString(bottom_text,
                                  BADGE_X + BADGE_W / 2,
                                  BADGE_Y + 22);
    }

    // 4. Push composited frame to the physical display
    _clock_sprite.pushSprite(0, 0);
}

void HomeScreen::DrawClock(const struct tm* timeinfo) {
    char time_str[6];
    char date_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d",
             timeinfo->tm_hour, timeinfo->tm_min);
    strftime(date_str, sizeof(date_str), "%a, %d %b", timeinfo);

    // Composite: clean background + events badge
    _bg_sprite.pushSprite(&_clock_sprite, 0, 0);
    DrawEventsBadge();

    // Badge background
    _clock_sprite.fillRoundRect(BADGE_X, BADGE_Y, BADGE_W, BADGE_H, BADGE_R,
                                 _clock_sprite.color565(10, 10, 10));
    _clock_sprite.setTextDatum(lgfx::MC_DATUM);

    // Time — large, upper portion of badge
    _clock_sprite.setFont(&lgfx::fonts::DejaVu40);
    _clock_sprite.setTextColor(TFT_WHITE);
    _clock_sprite.drawString(time_str, BADGE_X + BADGE_W / 2, BADGE_Y + 22);

    // Date — smaller, lower portion of badge
    _clock_sprite.setFont(&lgfx::fonts::DejaVu18);
    _clock_sprite.setTextColor(_clock_sprite.color565(180, 180, 180));
    _clock_sprite.drawString(date_str, BADGE_X + BADGE_W / 2, BADGE_Y + 50);

    _clock_sprite.pushSprite(0, 0);
}

// ─── Main loop ────────────────────────────────────────────────────────────────
void HomeScreen::RunDigitalClockBg() {
    _bg_sprite.setPsram(true);
    _bg_sprite.setColorDepth(16);
    if (!_bg_sprite.createSprite(CONFIG_DISPLAY_WIDTH, CONFIG_DISPLAY_HEIGHT)) {
        log_e("Failed to allocate background sprite");
        return;
    }
    _bg_sprite.fillSprite(TFT_BLACK);

    _clock_sprite.setPsram(true);
    _clock_sprite.setColorDepth(16);
    if (!_clock_sprite.createSprite(CONFIG_DISPLAY_WIDTH, CONFIG_DISPLAY_HEIGHT)) {
        log_e("Failed to allocate clock sprite");
        _bg_sprite.deleteSprite();
        return;
    }
    _clock_sprite.fillSprite(TFT_BLACK);

    // Collect JPEG files
    std::vector<std::string> jpeg_files;
    SDCARD_FileList(CONFIG_HOMESCREEN_FOLDER, jpeg_files, "jpg");
    std::vector<std::string> jpeg_ext;
    SDCARD_FileList(CONFIG_HOMESCREEN_FOLDER, jpeg_ext, "jpeg");
    jpeg_files.insert(jpeg_files.end(), jpeg_ext.begin(), jpeg_ext.end());
    log_i("HomeScreen: %d JPEG image(s) found", (int)jpeg_files.size());

    size_t      current_index    = 0;
    int         last_minute      = -1;
    NTPStatus_e last_status      = NTP_STATUS_IDLE;
    WiFiState_e last_wifi_state  = WIFI_STATE_IDLE;
    int         last_ev_version  = -1;
    int64_t     last_image_ms    = -(int64_t)CONFIG_HOMESCREEN_IMAGE_INTERVAL_MS;
    int         ip_ticks         = 0;   // countdown: show IP for N seconds after connect
    char        ip_display[20]   = {};

    NTPClient&      ntp    = NTPClient::GetInstance();
    IncomingEvents& events = IncomingEvents::GetInstance();
    WiFiManager&    wifi   = WiFiManager::GetInstance();

    while (true) {
        int64_t now_ms      = esp_timer_get_time() / 1000;
        bool    bg_reloaded = false;

        // ── Rotate background image ───────────────────────────────────────────
        if (!jpeg_files.empty() &&
            (now_ms - last_image_ms) >= (int64_t)CONFIG_HOMESCREEN_IMAGE_INTERVAL_MS) {

            if (LoadJpegToSprite(jpeg_files[current_index])) {
                log_i("Background: %s (%d/%d)",
                      jpeg_files[current_index].c_str(),
                      (int)(current_index + 1), (int)jpeg_files.size());
                bg_reloaded = true;
            }
            current_index = (current_index + 1) % jpeg_files.size();
            last_image_ms = now_ms;
        }

        // ── Detect WiFi connect → capture IP, start 5 s display ──────────────
        WiFiState_e wifi_state = wifi.GetState();
        if (wifi_state == WIFI_STATE_CONNECTED && last_wifi_state != WIFI_STATE_CONNECTED) {
            snprintf(ip_display, sizeof(ip_display), "%s", wifi.GetIP());
            ip_ticks = 5;
        }
        last_wifi_state = wifi_state;

        NTPStatus_e status         = ntp.GetStatus();
        int         ev_version     = events.GetVersion();
        bool        status_changed = (status != last_status);
        bool        events_changed = (ev_version != last_ev_version);
        bool        need_redraw    = bg_reloaded || status_changed || events_changed;

        // ── Show IP for 5 s immediately after WiFi connects ───────────────────
        if (ip_ticks > 0) {
            RenderFrame(ip_display, &lgfx::fonts::DejaVu18, TFT_GREEN);
            ip_ticks--;
            last_status     = status;
            last_ev_version = ev_version;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // ── Choose what to render based on NTP state ──────────────────────────
        switch (status) {
            case NTP_STATUS_WIFI_CONNECTING:
                if (need_redraw) {
                    bool ap = wifi_state == WIFI_STATE_AP_MODE;
                    RenderFrame(ap ? "Access Point" : "WiFi Connecting...",
                                &lgfx::fonts::DejaVu18,
                                ap ? TFT_CYAN : TFT_YELLOW);
                }
                break;

            case NTP_STATUS_NTP_SYNCING:
                if (need_redraw)
                    RenderFrame("Time Updating...", &lgfx::fonts::DejaVu18, TFT_YELLOW);
                break;

            case NTP_STATUS_SYNCED: {
                struct tm timeinfo = {};
                ntp.GetTime(&timeinfo);
                if (need_redraw || timeinfo.tm_min != last_minute) {
                    DrawClock(&timeinfo);
                    last_minute = timeinfo.tm_min;
                }
                break;
            }

            default:
                // IDLE or FAILED — background + events, no bottom badge
                if (need_redraw)
                    RenderFrame(nullptr, nullptr, 0);
                break;
        }

        last_status     = status;
        last_ev_version = ev_version;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
