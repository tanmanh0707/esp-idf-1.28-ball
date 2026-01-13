#pragma once
#include "config_app.h"

typedef enum {
  GIF_CMD_PLAY_PATH = (0),
  GIF_CMD_PLAY_RAM,
  GIF_CMD_MAX
} GifCmd_e;


typedef enum {
  GIF_IDLE = (0),
  GIF_PLAYING,
  GIF_PAUSED,
  GIF_READING,
  GIF_STATE_MAX
} GifState_e;

typedef enum {
  GIF_LOOP_NONE = (0),
  GIF_LOOP_INFINITE,
  GIF_LOOP_MAX
} GifLoop_e;

void GIF_Init();
