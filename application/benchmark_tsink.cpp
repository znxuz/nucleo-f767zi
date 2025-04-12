#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <printf.h>
#include <queue.h>
#include <semphr.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <threadsafe_sink.hpp>

using namespace freertos;

extern "C" {

static const char* lorem = "Lorem ipsum dolor sit amet, consectetur elit.";

SemaphoreHandle_t bench_semphr;

static QueueHandle_t benchmark_queue;

constexpr size_t BENCHMARK_N = 10;

void run_benchmark(void*) {
  auto time = DWT->CYCCNT;
  char buf[100];
  static std::atomic<size_t> ticket_machine;

  constexpr size_t iteration = 2000;
  for (size_t i = 0; i < iteration; ++i) {
    auto ticket = ticket_machine.fetch_add(1);
    auto size = snprintf(buf, sizeof(buf), "%u. ticket: %s\n", ticket, lorem);

    tsink_write_ordered(buf, size, ticket);
    // tsink_write_blocking(buf, size);
  }

  time = static_cast<double>(DWT->CYCCNT - time) / SystemCoreClock * 1000;
  xQueueSend(benchmark_queue, &time, 0);
  xSemaphoreGive(bench_semphr);

  while (true) {
    vTaskDelay(1000);
  }
}

void print_benchmark(void*) {
  static constexpr uint8_t configNUM_TASKS = BENCHMARK_N + 5;
  static char buf[50 * configNUM_TASKS];

  auto time = DWT->CYCCNT;
  for (size_t i = 0; i < BENCHMARK_N; ++i)
    xSemaphoreTake(bench_semphr, portMAX_DELAY);

  time = static_cast<double>(DWT->CYCCNT - time) / SystemCoreClock * 1000;
  tsink_write_str("===================================\n");
  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    size_t t;
    xQueueReceive(benchmark_queue, &t, 0);
    tsink_write_blocking(buf,
                         snprintf(buf, sizeof(buf), "time in ms: %u\n", t));
  }

  vTaskGetRunTimeStats(buf);
  tsink_write_str("===================================\n");
  tsink_write_str("Task\t\tTime\t\t%%\n");
  tsink_write_str(buf);

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
