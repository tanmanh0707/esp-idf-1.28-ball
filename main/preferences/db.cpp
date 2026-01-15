#include "db.h"
#include "utils.h"
#include "Preferences.h"
#include "lockguard.h"
#include "log_app.h"
#include "config_task.h"
#include "queue_app.h"

#define TAG "DB"
#define PREF_READONLY                               true
#define PREF_READWRITE                              false

Preferences preferences;
static Mutex _dbMutex;
static QueueApp db_queue("db");
static TaskHandle_t db_task_handle = nullptr;

static void DB_Task(void *pvParameters);

bool DB_Init()
{
  if (!preferences.init()) {
    log_e("Failed to initialize Preferences");
    return false;
  }

  // Create queue for DB operations
  if (db_queue.Create(TASK_DBASYNC_QUEUE_SIZE) == false) {
    log_e("Failed to create DB queue");
    return false;
  }

  // Create DB task pinned to core 0
  BaseType_t result = xTaskCreatePinnedToCore(
    DB_Task,
    "DB_Task",
    TASK_DBASYNC_STACK_SIZE,
    TASK_NO_PARAM,
    TASK_DBASYNC_PRIORITY,
    &db_task_handle,
    TASK_DBASYNC_CORE
    /* DO NOT move this task to PSRAM */
    /* nvs flash will not work in PSRAM stack */
  );

  if (result != pdPASS) {
    log_e("Failed to create DB task");
    return false;
  }

  log_i("DB Task initialized successfully");
  return true;
}

static void DB_Task(void *pvParameters)
{
  QueueApp::QueueMsg_st msg;

  log_i("DB Task started on core %d", xPortGetCoreID());

  while (1) {
    if (xQueueReceive(db_queue.QueueHandle(), &msg, portMAX_DELAY) == pdTRUE) {
      switch (msg.cmd) {
        default:
          log_w("Unknown DB command: %d", msg.cmd);
          break;
      }
    }

    if (msg.data) {
      free(msg.data);
      msg.data = NULL;
    }
  }
}