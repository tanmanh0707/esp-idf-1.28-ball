#pragma once
#include "config_app.h"

typedef enum {
  TEXT_ALIGN_LEFT = (0),
  TEXT_ALIGN_RIGHT,
  TEXT_ALIGN_CENTER
} TextAlign_e;

typedef enum {
  TEXT_SIZE_SMALL = (1),
  TEXT_SIZE_MEDIUM,
  TEXT_SIZE_BIG,
  TEXT_SIZE_LARGE
} TextSize_e;

void DISPLAY_FillCircle(int32_t x, int32_t y, int32_t r, int32_t color);
void DISPLAY_FillRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void DISPLAY_DrawText(const char *text,
                      uint16_t x = CONFIG_DISPLAY_WIDTH / 2,
                      uint16_t y = CONFIG_DISPLAY_HEIGHT / 2,
                      TextAlign_e align = TEXT_ALIGN_CENTER,
                      TextSize_e size = TEXT_SIZE_BIG,
                      uint16_t color = 0xFFFF,
                      uint16_t bg_color = 0x0000);

void DISPLAY_Init();
void DISPLAY_FillScreen(int32_t color);
void DISPLAY_ClearScreen();
void DISPLAY_SetAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h);
void DISPLAY_StartWriteDma(int32_t x, int32_t y, int32_t w, int32_t h);
void DISPLAY_WaitDMA();
void DISPLAY_EndWrite();
bool DISPLAY_PushPixels(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t *pixels);
void DISPLAY_PushPixelsDMA(const uint16_t* pixels, uint32_t len, bool swap = false);