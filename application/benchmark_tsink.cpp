#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <printf.h>
#include <queue.h>
#include <semphr.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <threadsafe_sink.hpp>

using namespace freertos;

extern "C" {

static const char* lorem = "Lorem ipsum dolor sit amet, consectetur elit.\n";

SemaphoreHandle_t bench_semphr;

static QueueHandle_t benchmark_queue;

constexpr size_t BENCHMARK_N = 5;

size_t prints(const char* format, ...) {
  static char buf[100];

  va_list args;
  va_start(args, format);
  auto size = vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  tsink::write(buf, size);

  return size;
}

// void _putchar(char c) {
//   tsink::write(&c, 1);
// }

void run_benchmark(void*) {
  auto time = DWT->CYCCNT;

  constexpr size_t iteration = 5000;
  for (size_t i = 0; i < iteration; ++i) { tsink::write_str(lorem); }

  time = static_cast<double>(DWT->CYCCNT - time) / SystemCoreClock * 1000;
  xQueueSend(benchmark_queue, &time, 0);
  xSemaphoreGive(bench_semphr);

  while (true) {
    vTaskDelay(1000);
  }
}

void print_benchmark(void*) {
  auto time = DWT->CYCCNT;
  for (size_t i = 0; i < BENCHMARK_N; ++i)
    xSemaphoreTake(bench_semphr, portMAX_DELAY);

  time = static_cast<double>(DWT->CYCCNT - time) / SystemCoreClock * 1000;
  tsink::write_str("==========================================\n");
  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    size_t t;
    xQueueReceive(benchmark_queue, &t, 0);
    prints("time in us: %u\n", t);
  }

  prints("time elapsed: %u\n", time);

  static constexpr uint8_t configNUM_TASKS = BENCHMARK_N + 5;
  static char stat_buf[50 * configNUM_TASKS];

  vTaskGetRunTimeStats(stat_buf);
  tsink::write_str("==========================================\n");
  prints("Task\t\tTime\t\t%%\n");
  tsink::write_str(stat_buf);

  while (true) {
    vTaskDelay(1000);
  }
}

void benchmark_tsink() {
  configASSERT((bench_semphr = xSemaphoreCreateCounting(BENCHMARK_N, 0)));
  configASSERT((benchmark_queue = xQueueCreate(BENCHMARK_N, sizeof(uint32_t))));

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
