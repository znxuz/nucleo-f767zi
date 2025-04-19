#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <printf/printf.h>
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
#include <freertos-threadsafe-sink/threadsafe_sink.hpp>
#include <string_view>

using namespace freertos;
using namespace std::string_view_literals;

static const char* lorem = "Lorem ipsum dolor sit amet, consectetur elit.";

SemaphoreHandle_t bench_semphr;

static QueueHandle_t benchmark_queue;

constexpr size_t BENCHMARK_N = 5;

void va_write(size_t ticket, const char* format, ...) {
  char buf[100];
  va_list args;
  va_start(args, format);
  auto size = vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  // tsink_write_ordered(buf, size, ticket);
  tsink::write_blocking(buf, size);
}

void run_benchmark(void*) {
  auto cycle = DWT->CYCCNT;
  static std::atomic<size_t> ticket_machine;

  constexpr size_t iteration = 5000;
  float f = 1.25;
  for (size_t i = 0; i < iteration; ++i) {
    auto ticket = ticket_machine.fetch_add(1, std::memory_order_acquire);
    va_write(ticket, "%u. ticket with f: %.2f: %s\n", ticket, f + i, lorem);
  }

  cycle = DWT->CYCCNT - cycle;
  xQueueSend(benchmark_queue, &cycle, portMAX_DELAY);
  xSemaphoreGive(bench_semphr);

  while (true) {
    vTaskDelay(1000);
  }
}

void print_benchmark(void*) {
  static constexpr uint8_t configNUM_TASKS = BENCHMARK_N + 5;
  static char buf[50 * configNUM_TASKS];

  for (size_t i = 0; i < BENCHMARK_N; ++i)
    xSemaphoreTake(bench_semphr, portMAX_DELAY);

  tsink::write_blocking("===================================\n"sv);
  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    size_t time;
    xQueueReceive(benchmark_queue, &time, 0);
    tsink::write_blocking(
        buf, snprintf(buf, sizeof(buf), "time in ms: %u\n",
                      static_cast<uint32_t>(static_cast<float>(time) /
                                            SystemCoreClock * 1000)));
  }

  vTaskGetRunTimeStats(buf);
  tsink::write_blocking("===================================\n"sv);
  tsink::write_blocking("Task\t\tTime\t\t%%\n"sv);
  tsink::write_blocking(buf, strlen(buf));

  while (true) {
    vTaskDelay(1000);
  }
}

void tsink_benchmark() {
  configASSERT((bench_semphr = xSemaphoreCreateCounting(BENCHMARK_N, 0)));
  configASSERT((benchmark_queue = xQueueCreate(BENCHMARK_N, sizeof(uint32_t))));

  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    configASSERT(xTaskCreate(run_benchmark, "benchmark",
                             configMINIMAL_STACK_SIZE * 2, NULL,
                             osPriorityNormal, NULL) == pdPASS);
  }
  configASSERT(xTaskCreate(print_benchmark, "print_bench",
                           configMINIMAL_STACK_SIZE, NULL, osPriorityNormal,
                           NULL) == pdPASS);
}
