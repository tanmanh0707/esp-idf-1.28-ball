#pragma once
#include "config_app.h"
#include "lockguard.h"

#define QUEUE_NAME_LEN              32

class QueueApp {
public:
  typedef uint16_t QueueCommand_t;

  typedef struct
  {
    QueueCommand_t cmd;
    uint8_t *data;
    uint16_t data_len;
  } QueueMsg_st;

  QueueApp(const char *name);
  ~QueueApp();

  bool Send(QueueCommand_t cmd, uint8_t *data = nullptr, uint16_t data_len = 0, bool copy_data = QUEUE_COPY_DATA, bool send_to_front = false);
  void Flush();
  private:
  char m_name[QUEUE_NAME_LEN] = {0};
  QueueHandle_t m_queue = nullptr;
  std::unique_ptr<Mutex> m_mutex = nullptr;
};