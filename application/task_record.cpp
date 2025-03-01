#include <FreeRTOS.h>
#include <main.h>
#include <queue.h>
#include <stm32f767xx.h>
#include <task.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "cmsis_os2.h"

struct TaskRecord {
  const char* name;
  uint32_t cycle_count;
  bool is_begin;
} __attribute__((packed));

static volatile bool task_switch_profiling_enabled;
static size_t ctx_switch_cnt;

// static TaskRecord task_records[10000];
// static volatile size_t task_records_idx;

QueueHandle_t task_record_queue;

struct cycle_stamp {
  cycle_stamp(std::string_view name)
      : task_record{name.data(), DWT->CYCCNT, true} {
    if (task_switch_profiling_enabled) {
      configASSERT(xQueueSend(task_record_queue, &task_record, 0));
      // taskENTER_CRITICAL();
      // task_records[task_records_idx++] = {name.data(), DWT->CYCCNT, true};
      // taskEXIT_CRITICAL();
    }
  }
  ~cycle_stamp() {
    if (task_switch_profiling_enabled) {
      task_record.cycle_count = DWT->CYCCNT;
      task_record.is_begin = false;
      configASSERT(xQueueSend(task_record_queue, &task_record, 0));
      // taskENTER_CRITICAL();
      // task_records[task_records_idx++] = {name.data(), DWT->CYCCNT, false};
      // taskEXIT_CRITICAL();
    }
  }

  TaskRecord task_record;
};

extern "C" {

TaskHandle_t button_task_handle;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  static constexpr uint8_t DEBOUNCE_TIME_MS = 200;
  static volatile uint32_t last_interrupt_time = 0;

  if (GPIO_Pin == USER_Btn_Pin) {
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_interrupt_time > DEBOUNCE_TIME_MS) {
      static BaseType_t xHigherPriorityTaskWoken;
      configASSERT(button_task_handle != NULL);
      vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

      last_interrupt_time = current_time;
    }
  }
}
void button_task(void*) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    taskENTER_CRITICAL();
    task_switch_profiling_enabled ^= 1;
    taskEXIT_CRITICAL();
  }
}

void task_switched_in_callback(const char* name) {
  if (!task_switch_profiling_enabled) return;

  auto record = TaskRecord{name, DWT->CYCCNT, true};
  configASSERT(xQueueSendFromISR(task_record_queue, &record, NULL));
  ++ctx_switch_cnt;

  // task_records[task_records_idx].is_begin = true;
  // task_records[task_records_idx].name = name;
  // task_records[task_records_idx++].timestamp = DWT->CYCCNT;
}

void task_switched_out_callback(const char* name) {
  if (!task_switch_profiling_enabled) return;

  auto record = TaskRecord{name, DWT->CYCCNT, false};
  configASSERT(xQueueSendFromISR(task_record_queue, &record, NULL));
  ++ctx_switch_cnt;

  // task_records[task_records_idx].name = name;
  // task_records[task_records_idx++].timestamp = DWT->CYCCNT;
}

void task1(void*) {
  // bool print_once = false;
  TaskRecord task_record;

  while (true) {
    while (xQueueReceive(task_record_queue, &task_record, 0) == pdPASS) {
      const auto& [name, cycle_count, is_begin] = task_record;
      printf("%s:\t%s\t%09lu\n", (is_begin ? "begin" : "end"), name,
             static_cast<unsigned long>(cycle_count));
    }
    if (ctx_switch_cnt) {
      printf("ctx switch count: %u\n", ctx_switch_cnt);
      ctx_switch_cnt = 0;
    }

    // if (!task_switch_profiling_enabled) {
    //   if (std::exchange(print_once, true)) continue;
    //
    //   taskENTER_CRITICAL();
    //   auto idx_cp = task_records_idx;
    //   taskEXIT_CRITICAL();
    //
    //   for (size_t i = 0; i < idx_cp; ++i) {
    //     const auto& [name, timestamp, is_begin] = task_records[i];
    //     printf("%s:\t%s\t%09u\n%s", (is_begin ? "begin" : "end"), name,
    //            timestamp, (i + 1 == idx_cp) ? "\n" : "");
    //   }
    // } else {
    //   print_once = false;
    // }
    //
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void task_button_init() {
  xTaskCreate(button_task, "button", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, &button_task_handle);
}

void task_record_init() {
  task_record_queue = xQueueCreate(1000, sizeof(TaskRecord));
  configASSERT(task_record_queue != NULL);
  xTaskCreate(task1, "task1", configMINIMAL_STACK_SIZE, NULL, osPriorityNormal,
              NULL);
}
}
