#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <queue.h>
#include <semphr.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "task_uart_streambuf.hpp"

extern "C" {
#ifndef USE_UART_STREAMBUF
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
#endif

static const char* lorem = "Lorem ipsum dolor sit amet, consectetur elit.";
// static const char* lorem = "Lorem.";

SemaphoreHandle_t printf_semphr;
SemaphoreHandle_t bench_semphr;

static QueueHandle_t benchmark_queue;

constexpr size_t BENCHMARK_N = 5;

int mtxprintf(const char* format, ...) {
  xSemaphoreTake(printf_semphr, portMAX_DELAY);
  va_list args;
  va_start(args, format);
  int result = std::vprintf(format, args);
  va_end(args);
  xSemaphoreGive(printf_semphr);

  return result;
}

void run_benchmark(void*) {
  auto start = DWT->CYCCNT;

  for (size_t i = 0; i < 5000; ++i)
    mtxprintf("%s\n", lorem);

  auto end = DWT->CYCCNT;
  uint32_t time_us = static_cast<double>(end - start) / SystemCoreClock * 1000;

  xQueueSend(benchmark_queue, &time_us, 0);
  xSemaphoreGive(bench_semphr);

  while (true) {
    vTaskDelay(1000);
  }
}

void print_benchmark(void*) {
  for (size_t i = 0; i < BENCHMARK_N; ++i)
    xSemaphoreTake(bench_semphr, portMAX_DELAY);

  size_t total = 0;
  size_t time_ms;
  puts("==========================================");
  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    xQueueReceive(benchmark_queue, &time_ms, 0);
    total += time_ms;
    mtxprintf("time in ms: %u\n", time_ms);
  }

  mtxprintf("total: %u\n", total);

  static constexpr uint8_t configNUM_TASKS = BENCHMARK_N + 5;
  static char stat_buf[40 * configMAX_TASK_NAME_LEN * configNUM_TASKS];

  vTaskGetRunTimeStats(stat_buf);
  puts("==========================================");
  mtxprintf("Task\t\tTime\t\t%%\n");
  mtxprintf("%s\n", stat_buf);

  while (true) {
    vTaskDelay(1000);
  }
}

void benchmark_streambuf() {
  configASSERT((printf_semphr = xSemaphoreCreateMutex()));
  configASSERT((bench_semphr = xSemaphoreCreateCounting(BENCHMARK_N, 0)));
  configASSERT((benchmark_queue = xQueueCreate(BENCHMARK_N, sizeof(uint32_t))));

#ifdef USE_UART_STREAMBUF
  task_uart_streambuf_init();
#endif

  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    configASSERT(xTaskCreate(run_benchmark, "benchmark",
                             configMINIMAL_STACK_SIZE, NULL, osPriorityNormal,
                             NULL) == pdPASS);
  }
  configASSERT(xTaskCreate(print_benchmark, "print_bench",
                           configMINIMAL_STACK_SIZE, NULL, osPriorityNormal,
                           NULL) == pdPASS);
}
}
