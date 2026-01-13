#include "system_info.h"
#include "log_app.h"
#include <esp_system.h>
#include "config_app.h"

#define TAG "SYS"

esp_reset_reason_t SYSTEM_ResetReason()
{
  esp_reset_reason_t reset_reason = esp_reset_reason();

  switch (reset_reason) {
  case ESP_RST_POWERON:
    log_i("Reset reason: Power-on reset");
    break;
  case ESP_RST_EXT:
    log_i("Reset reason: External reset (reset pin)");
    break;
  case ESP_RST_SW:
    log_i("Reset reason: Software reset");
    break;
  case ESP_RST_PANIC:
    log_i("Reset reason: Exception/Panic reset");
    break;
  case ESP_RST_INT_WDT:
    log_i("Reset reason: Interrupt watchdog reset");
    break;
  case ESP_RST_TASK_WDT:
    log_i("Reset reason: Task watchdog reset");
    break;
  case ESP_RST_WDT:
    log_i("Reset reason: Other watchdog reset");
    break;
  case ESP_RST_DEEPSLEEP:
    log_i("Reset reason: Wakeup from deep sleep");
    break;
  case ESP_RST_BROWNOUT:
    log_i("Reset reason: Brownout reset (low voltage)");
    break;
  case ESP_RST_SDIO:
    log_i("Reset reason: SDIO reset");
    break;
  default:
    log_i("Reset reason: Unknown");
    break;
  }

  return reset_reason;
}

void SYSTEM_Restart(bool direct)
{
  if (direct == false) {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
  esp_restart();
}