#include "application.hpp"

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <errno.h>
#include <queue.h>
#include <stdio.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <utility>

typedef struct {
  const char* name;
  uint32_t start_count;
  uint32_t end_count;
} TaskSwitchRecord_t;

static volatile TaskSwitchRecord_t task_records[10000];
static volatile size_t task_records_idx;
static volatile bool enable_task_switched_hook;

struct timestamp {
  ~timestamp() {
    printf("%.07f\n",
           static_cast<double>(DWT->CYCCNT - start) / SystemCoreClock * 1000);
  }
  uint32_t start = DWT->CYCCNT;
};

extern "C" {
int _write(int file, char* ptr, int len) {
  HAL_StatusTypeDef hstatus;

  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    hstatus = HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);

    if (hstatus == HAL_OK)
      return len;
    else
      return EIO;
  }
  errno = EBADF;
  return -1;
}

static void enable_dwt_cycle_count() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;  // software unlock
  DWT->CYCCNT = 1;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

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
    enable_task_switched_hook ^= 1;
    taskEXIT_CRITICAL();
  }
}

void task_switched_in_callback(const char* name) {
  if (!enable_task_switched_hook) return;

  task_records[task_records_idx].name = name;
  task_records[task_records_idx].start_count = DWT->CYCCNT;
}

void task_switched_out_callback() {
  if (task_records[task_records_idx].name == NULL) return;

  task_records[task_records_idx++].end_count = DWT->CYCCNT;
}

void task1(void*) {
  bool print_once = false;
  while (true) {
    if (!enable_task_switched_hook) {
      if (std::exchange(print_once, true)) continue;

      taskENTER_CRITICAL();
      auto idx_cp = task_records_idx;
      taskEXIT_CRITICAL();

      for (size_t i = 0; i < idx_cp; ++i) {
        const auto& [name, start, end] = task_records[i];
        printf("%s:\t[%09u\t%09u]\n%s", name, start, end,
               (i + 1 == idx_cp) ? "\n" : "");
      }
    } else {
      print_once = false;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
}

void application_start() {
  enable_dwt_cycle_count();

  xTaskCreate(button_task, "button", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, &button_task_handle);
  xTaskCreate(task1, "task1", configMINIMAL_STACK_SIZE, NULL, osPriorityNormal,
              NULL);

  puts("kernel start");
  osKernelStart();
}
