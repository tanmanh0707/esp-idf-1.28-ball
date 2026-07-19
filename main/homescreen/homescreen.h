#pragma once
#include "config_app.h"
#include "LovyanGFX.hpp"
#include <string>
#include <vector>
#include <ctime>

typedef enum {
    HOMESCREEN_TYPE_DIGITAL_CLOCK_BG = 0,
} HomeScreenType_e;

class HomeScreen {
public:
    explicit HomeScreen(lgfx::LGFX_Device* lcd,
                        HomeScreenType_e type = HOMESCREEN_TYPE_DIGITAL_CLOCK_BG);
    ~HomeScreen();

    void Start();

private:
    void RunDigitalClockBg();
    bool LoadJpegToSprite(const std::string& filename);

    // Composites bg + event badge + bottom badge into clock_sprite, then pushes to LCD
    void RenderFrame(const char* bottom_text,
                     const lgfx::IFont* bottom_font,
                     uint16_t bottom_color);
    void DrawEventsBadge();
    void DrawClock(const struct tm* timeinfo);

    lgfx::LGFX_Device* _lcd;
    HomeScreenType_e    _type;
    lgfx::LGFX_Sprite   _bg_sprite;    // clean decoded background (never pushed directly)
    lgfx::LGFX_Sprite   _clock_sprite; // composited output frame
};
