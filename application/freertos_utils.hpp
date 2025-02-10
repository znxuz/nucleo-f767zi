#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

#include <string_view>

namespace freertos {

class AbstractThread {
 public:
  AbstractThread(std::string_view name, uint16_t stack_depth,
                 UBaseType_t priority, void* arg, bool auto_start)
      : adapter_args{this, arg},
        name(name),
        stack_depth(stack_depth),
        priority(priority) {
    if (auto_start) this->start();
  }

  virtual ~AbstractThread() { vTaskDelete(handle); }

  AbstractThread(const AbstractThread&) = delete;
  AbstractThread(AbstractThread&&) = delete;
  AbstractThread operator=(const AbstractThread&) = delete;
  AbstractThread operator=(AbstractThread&&) = delete;

  bool start() {
    return xTaskCreate(adapter_fn, name.data(), stack_depth,
                       static_cast<void*>(&adapter_args), priority,
                       &handle) == pdPASS;
  }

 private:
  struct AdapterArgs {
    AbstractThread* thread;
    void* arg;
  };

  AdapterArgs adapter_args;
  TaskHandle_t handle;
  std::string_view name;
  uint16_t stack_depth;
  UBaseType_t priority;

  virtual void run(void* arg) = 0;  // endless loop impl.

  static void adapter_fn(void* adapter_args) {
    auto* args = static_cast<struct AdapterArgs*>(adapter_args);
    args->thread->run(args->arg);
  }
};

class Timer {
 public:
  Timer(TickType_t duration)
      : prev_time{xTaskGetTickCount()}, duration{duration} {}

  void wait() { vTaskDelayUntil(&prev_time, duration); }

  void wait_until(TickType_t duration) {
    vTaskDelayUntil(&prev_time, duration);
  }

 private:
  TickType_t prev_time;
  TickType_t duration;
};

class Semaphore {
 public:
  Semaphore(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount) {
    this->handle = xSemaphoreCreateCounting(uxMaxCount, uxInitialCount);
  }

  ~Semaphore() { vSemaphoreDelete(this->handle); }

  bool wait() { return xSemaphoreTake(this->handle, portMAX_DELAY); }

  bool signal() { return xSemaphoreGive(this->handle); }

 private:
  SemaphoreHandle_t handle = nullptr;
};

class Mutex {
 public:
  Mutex() { this->handle = xSemaphoreCreateMutex(); }

  ~Mutex() { vSemaphoreDelete(this->handle); }

  bool lock() { return xSemaphoreTake(this->handle, portMAX_DELAY); }

  bool lock(TickType_t xTicksToWait) {
    return xSemaphoreTake(this->handle, xTicksToWait);
  }

  bool unlock() { return xSemaphoreGive(this->handle); }

 private:
  SemaphoreHandle_t handle;
};

// TODO refactor
template <typename T>
class MutexObject {
 public:
  MutexObject() = default;

  MutexObject(T t) { this->set(t); }

  T get() {
    T obj;
    mutex.lock();
    obj = this->object;
    mutex.unlock();
    return obj;
  }

  void set(T obj) {
    mutex.lock();
    this->object = obj;
    mutex.unlock();
  }

 private:
  T object{};
  Mutex mutex{};
};

};  // namespace freertos
