#include "queue_app.h"
#include "log_app.h"

#define TAG "QUEUE"

QueueApp::QueueApp(const char *name, uint32_t queue_size)
{
  strncpy(m_name, name, QUEUE_NAME_LEN);
  m_name[QUEUE_NAME_LEN - 1] = '\0';

  m_mutex = std::make_unique<Mutex>();

  if (queue_size > 0) {
    m_queue = xQueueCreate(queue_size, sizeof(QueueMsg_st));
    if (m_queue == nullptr) {
      log_e("Failed to create queue: %s", m_name);
    }
  } else {
    m_queue = nullptr;
  }
}

QueueApp::~QueueApp()
{
  if (m_queue) {
    vQueueDelete(m_queue);
    m_queue = nullptr;
  }
}

bool QueueApp::Create(uint32_t queue_size)
{
  bool res = false;

  if (queue_size > 0) {
    LockGuard lock(m_mutex);

    if (m_queue) {
      Flush();
      vQueueDelete(m_queue);
      m_queue = nullptr;
    }

    m_queue = xQueueCreate(queue_size, sizeof(QueueMsg_st));
    if (m_queue == nullptr) {
      log_e("Failed to create queue: %s", m_name);
    } else {
      res = true;
    }
  } else {
    log_e("Invalid queue size (%d): %s", queue_size, m_name);
  }

  return res;
}

bool QueueApp::Send(QueueCommand_t cmd, uint8_t *data, uint16_t data_len, bool copy_data, bool send_to_front)
{
  bool res = false;
  if (m_queue && m_mutex)
  {
    QueueMsg_st msg;
    msg.cmd = cmd;
    msg.data = data;
    msg.data_len = data_len;

    if (copy_data) {
      if (data && data_len) {
        msg.data = (uint8_t *)calloc(1, data_len);
        if (msg.data) {
          memcpy(msg.data, data, data_len);
        } else {
          log_e("%s - Failed to allocate memory message data", m_name);
          return res;
        }
      }
    }

    BaseType_t ret = pdFALSE;
    {
      LockGuard lock(m_mutex);
      if ( ! send_to_front) {
        ret = xQueueSend(m_queue, &msg, pdMS_TO_TICKS(1000));
      } else {
        ret = xQueueSendToFront(m_queue, &msg, pdMS_TO_TICKS(1000));
      }
    }

    if (ret != pdTRUE) {
      log_e("%s - Failed to send message to queue", m_name);
      if (copy_data && msg.data) {
        free(msg.data);
      }
    } else {
      res = true;
    }
  } else {
    log_e("%s - Queue not initialized", m_name);
  }

  return res;
}

void QueueApp::Flush()
{
  if (m_queue && m_mutex) {
    LockGuard lock(m_mutex);
    QueueMsg_st msg;
    while (xQueueReceive(m_queue, &msg, 0)) {
      if (msg.data) {
        free(msg.data);
      }
    }
  }
}