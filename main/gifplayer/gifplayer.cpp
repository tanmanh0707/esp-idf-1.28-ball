#include "gifplayer.h"
#include "AnimatedGIF.h"
#include "queue_app.h"
#include "config_task.h"
#include "utils.h"
#include "display.h"

#define TAG "GIF"

static AnimatedGIF _gif;
static QueueApp _gifQ("gifQ");
static GifState_e _state = GIF_IDLE;
static GifLoop_e _loop = GIF_LOOP_NONE;
static void *_framebuff = NULL;

static int16_t _drawX;
static int16_t _drawY;

static void gifplayer_task(void *arg);

void GIF_Init()
{
  if (_gifQ.Create(TASK_GIFPLAYER_QUEUE_SIZE) == false) {
    log_e("Gif Queue Create Failed!");
    SYSTEM_Restart();
  }

  BaseType_t ret = xTaskCreatePinnedToCore( gifplayer_task, "gifplayer",
                                            TASK_GIFPLAYER_STACK_SIZE,
                                            TASK_NO_PARAM,
                                            TASK_GIFPLAYER_PRIORITY,
                                            TASK_NO_HANDLER,
                                            TASK_GIFPLAYER_CORE);
  if (ret != pdTRUE) {
    log_e("GifPlayer Task Create failed!");
    SYSTEM_Restart();
  }
}

static void LocalGifChangeState(GifState_e new_state)
{
  if (new_state != _state) {
    _state = new_state;
    log_i("GIF State: %d", _state);
  }
}

static void GIF_End()
{
  _gif.close();
  LocalGifChangeState(GIF_IDLE);
}

void gifplayer_task(void *arg)
{
  const size_t FRAME_BUFF_SIZE = CONFIG_DISPLAY_WIDTH * CONFIG_DISPLAY_HEIGHT * 3;
  QueueApp::QueueMsg_st msg;
    
  if (_framebuff == nullptr) {
    _framebuff = ps_malloc(FRAME_BUFF_SIZE);
    if (_framebuff == nullptr) {
      log_e("Failed to allocate %zu bytes for frame buffer", FRAME_BUFF_SIZE);
      SYSTEM_Restart();
    }
  }

  while (1)
  {
    if (xQueueReceive(_gifQ.QueueHandle(), &msg, (_state == GIF_IDLE)? portMAX_DELAY : pdMS_TO_TICKS(10)) == pdTRUE) {
      switch (msg.cmd)
      {
        case GIF_CMD_PLAY_PATH:
          if (_state != GIF_IDLE) {
            GIF_End();
          }

          LocalGifChangeState(GIF_READING);
          break;


        case GIF_CMD_PLAY_RAM:
          break;

        default: break;
      }
    }

    if (msg.cmd == GIF_PLAYING) {
      if (_gif.playFrame(true, 0) > 0) {
        uint16_t *pPixels;
        int iX, iY, fW, fH;
        int w = _gif.getCanvasWidth();
        int h = _gif.getCanvasHeight();
        iX = _gif.getFrameXOff();
        iY = _gif.getFrameYOff();
        fW = _gif.getFrameWidth();
        fH = _gif.getFrameHeight();
  
        pPixels = (uint16_t *)((uint8_t *)_framebuff + (w * h));
        pPixels += iX + (iY * w);
        
        int16_t screenX = _drawX + iX;
        int16_t screenY = _drawY + iY;

        DISPLAY_StartWriteDma(screenX, screenY, fW, fH);
        pPixels = (uint16_t *)((uint8_t *)_framebuff + (w * h));
        pPixels += iX + (iY * w);
        for (int y = 0; y < fH; y++) {
          DISPLAY_PushPixelsDMA(pPixels, fW);
          pPixels += w;
        }
      } else {
        if (_loop == GIF_LOOP_INFINITE) {
          _gif.reset();
        } else {
          GIF_End();
        }
      }
    }
  }
}