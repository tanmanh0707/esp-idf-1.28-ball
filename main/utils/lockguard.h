#pragma once

#include "config_app.h"

class Mutex {
 public:
  Mutex() { m_handle = xSemaphoreCreateMutex(); }
  Mutex(const Mutex &) = delete;
  ~Mutex() {}
  void lock() { xSemaphoreTake(this->m_handle, portMAX_DELAY); }
  bool try_lock() { return xSemaphoreTake(this->m_handle, 0) == pdTRUE; }
  void unlock() { xSemaphoreGive(this->m_handle); }

  Mutex &operator=(const Mutex &) = delete;

 private:
  SemaphoreHandle_t m_handle;
};

class LockGuard {
public:
  LockGuard(std::unique_ptr<Mutex> &mutex) : m_mutex(mutex) { m_mutex->lock(); }
  ~LockGuard() { m_mutex->unlock(); }

private:
  std::unique_ptr<Mutex> &m_mutex;
};