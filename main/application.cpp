#include "application.h"
#include "log_app.h"
#include "display.h"
#include "gifplayer.h"
#include "sdcard.h"
#include "db.h"

#define TAG "APP"

Application::Application()
{
}

Application::~Application() {
}

void Application::Start() {
  DB_Init();
  DISPLAY_Init();
  GIF_Init();
  SDCARD_Init();
  DISPLAY_ClearScreen();
  log_i("Application Started!");

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
