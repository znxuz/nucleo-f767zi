#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <semphr.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>

TaskHandle_t task_handle;

static constexpr size_t BUF_SIZE = 1024;
static uint8_t ring_buf[BUF_SIZE];

static volatile uint8_t read_iteration[BUF_SIZE];
static volatile uint8_t write_iteration;

static volatile size_t read_idx;
static volatile size_t write_idx;

extern "C" {
#ifdef USE_UART_DMA
int _write(int file, char* ptr, int len) {
  for (size_t i = 0; i < len; ++i) {
    if (!write_iteration)
      while (write_iteration != read_iteration[write_idx]);
    else
      while (write_iteration > read_iteration[write_idx]);

    taskENTER_CRITICAL();
    ring_buf[write_idx] = ptr[i];
    write_idx += 1;
    write_iteration += (write_idx == BUF_SIZE);
    write_idx %= BUF_SIZE;
    taskEXIT_CRITICAL();
  }

  return len;
}
#endif

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance != huart3.Instance) return;

  static BaseType_t xHigherPriorityTaskWoken;
  vTaskNotifyGiveFromISR(task_handle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void task_uart_dma(void*) {
  while (true) {
    auto end = write_idx;  // atomic by nature on ARM

    if (read_idx == end) continue;
    if (read_idx < end) {
      size_t size = end - read_idx;
      HAL_UART_Transmit_DMA(&huart3, ring_buf + read_idx, size);
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      for (size_t i = read_idx; i < end; ++i) read_iteration[i] += 1;
      read_idx = end;
    } else {
      size_t size = BUF_SIZE - read_idx;
      HAL_UART_Transmit_DMA(&huart3, ring_buf + read_idx, size);
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      for (size_t i = read_idx; i < BUF_SIZE; ++i) read_iteration[i] += 1;
      read_idx = 0;
      if (end) {
        HAL_UART_Transmit_DMA(&huart3, ring_buf, end);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        for (size_t i = read_idx; i < end; ++i) read_iteration[i] += 1;
        read_idx = end;
      }
    }
  }
}

void task_uart_dma_init() {
  configASSERT(xTaskCreate(task_uart_dma, "uart_dma",
                           configMINIMAL_STACK_SIZE * 4, NULL, osPriorityNormal,
                           &task_handle) == pdPASS);
}
}
