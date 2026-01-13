#pragma once
#include "config_app.h"

void DISPLAY_Init();
void DISPLAY_FillScreen();
void DISPLAY_ClearScreen();
void DISPLAY_SetAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h);
void DISPLAY_StartWriteDma(int32_t x, int32_t y, int32_t w, int32_t h);
void DISPLAY_WaitDMA();
void DISPLAY_EndWrite();
void DISPLAY_PushPixelsDMA(const uint16_t* pixels, uint32_t len, bool swap = false);