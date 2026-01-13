#include "application.h"
#include "log_app.h"
#include "display.h"
#include "gifplayer.h"

#define TAG "APP"

Application::Application()
{
}

Application::~Application() {
}

void Application::Start() {
  DISPLAY_Init();
  GIF_Init();
  log_i("Application Started!");

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
