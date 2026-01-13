#include "display.h"
#include "LovyanGFX.hpp"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_GC9A01    _panel_instance;
  lgfx::Bus_SPI         _bus_instance;
  lgfx::Light_PWM       _light_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.freq_write = 40000000;
      cfg.pin_sclk = CONFIG_DISPLAY_SPI_SCLK_PIN;
      cfg.pin_mosi = CONFIG_DISPLAY_SPI_MOSI_PIN;
      cfg.pin_miso = -1;
      cfg.pin_dc = CONFIG_DISPLAY_SPI_DC_PIN;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Configure display panel
      auto cfg = _panel_instance.config();
      cfg.pin_cs = CONFIG_DISPLAY_SPI_CS_PIN;    // No CS pin required for parallel mode
      cfg.pin_rst = CONFIG_DISPLAY_SPI_RESET_PIN;    // Reset pin
      cfg.invert = true;  // Set `true` if colors are inverted
      cfg.rgb_order = false; // RGB or BGR order
      cfg.dlen_16bit = false; // Use 8-bit mode
      cfg.bus_shared = false; // Parallel bus is not shared
      cfg.offset_x = CONFIG_DISPLAY_OFFSET_X;
      cfg.offset_y = CONFIG_DISPLAY_OFFSET_Y;
      cfg.memory_width = CONFIG_DISPLAY_WIDTH;
      cfg.memory_height = CONFIG_DISPLAY_HEIGHT;
      cfg.panel_width = CONFIG_DISPLAY_WIDTH;
      cfg.panel_height = CONFIG_DISPLAY_HEIGHT;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = CONFIG_DISPLAY_BACKLIGHT_PIN;
      cfg.freq   = 5000;
      cfg.pwm_channel = 3;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX _lcd;

void DISPLAY_Init()
{
  _lcd.init();
  _lcd.setBrightness(200);

  DISPLAY_ClearScreen();
}

void DISPLAY_ClearScreen()
{
  _lcd.fillScreen(TFT_BLACK);
}

void DISPLAY_SetAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h)
{
  _lcd.setAddrWindow(x, y, w, h);
}

void DISPLAY_StartWriteDma(int32_t x, int32_t y, int32_t w, int32_t h)
{
  _lcd.setAddrWindow(x, y, w, h);
  _lcd.startWrite();
}

void DISPLAY_WaitDMA() {
}

void DISPLAY_EndWrite()
{
  _lcd.endWrite();
}

void DISPLAY_PushPixelsDMA(const uint16_t* pixels, uint32_t len, bool swap)
{
  _lcd.pushPixelsDMA(pixels, len, swap);
}