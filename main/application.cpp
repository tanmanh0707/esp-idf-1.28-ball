#include "application.h"
#include "log_app.h"
#include <esp_log.h>
#include "gifplayer.h"
#include "sdcard.h"
#include "speaker.h"
#include "db.h"
#include "wifi.h"
#include "ntpclient.h"
#include "incoming_events.h"
#include "homescreen.h"   // includes LovyanGFX.hpp
#include "display.h"      // must come after homescreen.h (avoids millis macro conflict)

#define TAG "APP"

Application::Application() {}
Application::~Application() {}

void Application::Start() {
    DB_Init();
    Speaker::GetInstance()->GetAudioCodec()->Start();
    DISPLAY_Init();
    GIF_Init();
    SDCARD_Init();
    DISPLAY_ClearScreen();

    log_i("Application started");
#if 0
    esp_log_level_set("esp_netif_sntp", ESP_LOG_DEBUG);
#endif
    WiFiManager::GetInstance().Init();
    NTPClient::GetInstance().Init();
    IncomingEvents::GetInstance().Init();

    HomeScreen homescreen(static_cast<lgfx::LGFX_Device*>(DISPLAY_GetDevice()));
    homescreen.Start();
}
