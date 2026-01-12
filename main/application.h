#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include <string>
#include <mutex>
#include <deque>
#include <memory>

#include "config_app.h"
#include "button.h"

class Application {
public:
  static Application& GetInstance() {
      static Application instance;
      return instance;
  }

  void Start();

private:
  Application();
  ~Application();

private:

};

#endif // _APPLICATION_H_
