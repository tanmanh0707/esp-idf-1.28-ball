#include "gifplayer.h"
#include "AnimatedGIF.h"
#include "queue_app.h"
#include "config_task.h"
#include "utils.h"
#include "display.h"
#include "sdcard.h"

#define TAG "GIF"

static AnimatedGIF _gif;
static QueueApp _gifQ("gifQ");
static GifState_e _state = GIF_IDLE;
static GifLoop_e _loop = GIF_LOOP_NONE;
static void *_framebuff = NULL;
static uint8_t *_file_buffer = NULL;
static size_t _file_buffer_size = 0;

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
  log_i("GIF End");
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
        {
          if (_state != GIF_IDLE) {
            GIF_End();
          }

          log_i("Request playing: %.*s", msg.data_len, msg.data);
          std::string file_path = std::string((const char *)msg.data, msg.data_len);
          size_t file_size = 0;
          LocalGifChangeState(GIF_READING);
          SDCARD_GetFileSize(file_path.c_str(), file_size);

          if (file_size) {
            if (_file_buffer) {
              if (_file_buffer_size < file_size) {
                ps_free(_file_buffer);
                _file_buffer_size = 0;
                _file_buffer = nullptr;
              }
            } 
            
            if (_file_buffer == nullptr) {
              _file_buffer = (uint8_t *)ps_malloc(file_size);
              if (_file_buffer) {
                _file_buffer_size = file_size;
              } else {
                log_e("Allocate file buffer failed! errno: %d", errno);
                _file_buffer_size = 0;
              }
            }

            bool ret = false;
            if (_file_buffer && _file_buffer_size) {
              if (SDCARD_ReadFile(file_path.c_str(), &_file_buffer, _file_buffer_size) == ESP_OK) {
                _gif.begin(BIG_ENDIAN_PIXELS);
                if (_gif.open(_file_buffer, _file_buffer_size, NULL)) {
                  if (_drawX < 0) {
                    _drawX = (CONFIG_DISPLAY_WIDTH - _gif.getCanvasWidth())/2;
                    if (_drawX < 0) { _drawX = 0; }
                  }
                  if (_drawY < 0) {
                    _drawY = (CONFIG_DISPLAY_HEIGHT - _gif.getCanvasHeight())/2;
                    if (_drawY < 0) { _drawY = 0; }
                  }
                  if (_drawX > 0 || _drawY > 0) {
                    DISPLAY_ClearScreen();
                  }

                  GIFINFO gifInfo;
                  _gif.getInfo(&gifInfo);
                  log_i("Frame num: %d", gifInfo.iFrameCount);
                  _gif.setDrawType(GIF_DRAW_COOKED);
                  _gif.setFrameBuf(_framebuff);
                  ret = true;
                } else {
                  log_e("GIF open failed!");
                }
              }
            }

            if (ret) {
              log_i("GIF Playing loop");
              _loop = GIF_LOOP_INFINITE;
              LocalGifChangeState(GIF_PLAYING);
            } else {
              LocalGifChangeState(GIF_IDLE);
            }
          } else {
            log_e("Read gif file failed!");
          }
        }
        break;

        case GIF_CMD_PLAY_RAM:
          break;

        default: break;
      }
    }

    static uint16_t pPixelsPingPong[2][1024];
    if (_state == GIF_PLAYING) {
      if (_gif.playFrame(false, 0) > 0) {
        int h, w;
        int16_t drawX_ = 0, drawY_ = 0;
        w = _gif.getCanvasWidth();
        h = _gif.getCanvasHeight();
        uint16_t *pPixels;
        int iX, iY, fW, fH;
        iX = _gif.getFrameXOff();
        iY = _gif.getFrameYOff();
        fW = _gif.getFrameWidth();
        fH = _gif.getFrameHeight();
  
        pPixels = (uint16_t *)((uint8_t *)_framebuff + (w * h));
        pPixels += iX + (iY * w);
        
        int16_t screenX = drawX_ + iX;
        int16_t screenY = drawY_ + iY;
        
        // Ping-pong buffer rendering
        size_t lineSize = fW * sizeof(uint16_t);
        uint16_t *pPixelsLine = pPixels;
        memcpy(pPixelsPingPong[0], pPixelsLine, lineSize);
        pPixelsLine += w;
        
        int currentBuffer = 0;
        int nextBuffer = 1;
        
        for (int16_t y = 0; y < fH; y++) {
            int16_t currentScreenY = screenY + y;
            DISPLAY_PushPixelsLineByLine(screenX, currentScreenY, fW, 1, pPixelsPingPong[currentBuffer]);
            if (y + 1 < fH) {
                memcpy(pPixelsPingPong[nextBuffer], pPixelsLine, lineSize);
                pPixelsLine += w;
            }
            
            std::swap(currentBuffer, nextBuffer);
        }
      } else {
        if (_loop == GIF_LOOP_INFINITE) {
          _gif.reset();
        } else {
          log_i("Gif finished!");
          GIF_End();
        }
      }
    }
  }
}

void GIF_PlayPath(const char *path)
{
  _gifQ.Send(GIF_CMD_PLAY_PATH, (uint8_t *)path, strlen(path));
}