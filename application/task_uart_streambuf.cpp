#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <semphr.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdarg>

#include <printf.h>

static SemaphoreHandle_t task_semphr;
static TaskHandle_t task_hdl;

static constexpr size_t BUF_SIZE = 2048;
static uint8_t ring_buf[BUF_SIZE];
static volatile size_t write_idx;

static uint8_t read_iteration[BUF_SIZE];

extern "C" {
void uart_write(const char* ptr, size_t len) {
  static uint8_t write_iteration;
  for (size_t i = 0; i < len; ++i) {
    if (!write_iteration)
      while (write_iteration != read_iteration[write_idx]) vTaskDelay(1);
    else
      while (write_iteration > read_iteration[write_idx]) vTaskDelay(1);
    ring_buf[write_idx] = ptr[i];

    taskENTER_CRITICAL();
    write_idx = (write_idx + 1) % BUF_SIZE;
    write_iteration += !write_idx;
    taskEXIT_CRITICAL();

    xSemaphoreGive(task_semphr);
  }
}

#ifdef USE_UART_STREAMBUF
void _putchar(char c) { uart_write((const char*)&c, 1); }
#endif

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance != huart3.Instance) return;

  static BaseType_t xHigherPriorityTaskWoken;
  vTaskNotifyGiveFromISR(task_hdl, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void task_uart_streambuf(void*) {
  size_t read_idx = 0;
  while (true) {
    xSemaphoreTake(task_semphr, portMAX_DELAY);

    auto end = write_idx;
    if (read_idx == end) continue;  // just for safety
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

void task_uart_streambuf_init() {
  configASSERT((task_semphr = xSemaphoreCreateBinary()));
  configASSERT((xTaskCreate(task_uart_streambuf, "uart_streambuf",
                            configMINIMAL_STACK_SIZE * 4, NULL,
                            osPriorityNormal, &task_hdl) == pdPASS));
}
}
