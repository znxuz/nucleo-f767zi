#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <queue.h>
#include <semphr.h>
#include <stm32f767xx.h>
#include <task.h>
#include <tim.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

extern "C" {
#ifndef USE_UART_DMA
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

volatile unsigned long ulHighFrequencyTimerTicks;

void configureTimerForRunTimeStats(void) {
  ulHighFrequencyTimerTicks = 0;
  HAL_TIM_Base_Start_IT(&htim13);
}

unsigned long getRunTimeCounterValue(void) { return ulHighFrequencyTimerTicks; }

void task_uart_dma_init();
void task_record_init();
void task_button_init();
void uart_transmit_string(const char* ptr, size_t len);

static void enable_dwt_cycle_count() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;  // software unlock
  DWT->CYCCNT = 1;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static const char* lorem = "Lorem ipsum dolor sit amet, consectetur elit.";
// static const char* lorem = "Lorem.";

SemaphoreHandle_t printf_semphr;
SemaphoreHandle_t bench_semphr;

static QueueHandle_t benchmark_queue;

constexpr size_t BENCHMARK_N = 3;

void run_benchmark(void*) {
  auto start = DWT->CYCCNT;

  for (size_t i = 0; i < 5000; ++i) {
    do {
      // must check the return value and retry like this, because it could fail
      if (xSemaphoreTake(printf_semphr, portMAX_DELAY) == pdTRUE) {
        printf("%s\n", lorem);
        xSemaphoreGive(printf_semphr);
        break;
      }
    } while (true);
  }

  auto end = DWT->CYCCNT;
  uint32_t time_us = static_cast<double>(end - start) / SystemCoreClock * 1000;

  xQueueSend(benchmark_queue, &time_us, 0);
  xSemaphoreGive(bench_semphr);

  while (true);
}

void print_benchmark(void*) {
  for (size_t i = 0; i < BENCHMARK_N; ++i)
    xSemaphoreTake(bench_semphr, portMAX_DELAY);

  size_t total = 0;
  size_t time_ms;
  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    xQueueReceive(benchmark_queue, &time_ms, 0);
    total += time_ms;
    printf("time in ms: %u\n", time_ms);
  }

  printf("total: %u\n", total);

  static constexpr uint8_t configNUM_TASKS = 7;
  static char stat_buf[40 * configMAX_TASK_NAME_LEN * configNUM_TASKS];

  vTaskGetRunTimeStats(stat_buf);
  puts("==========================================");
  printf("Task\t\tTime\t\t%%\n");
  printf("%s\n", stat_buf);

  while (true);
}

void application_start() {
  setvbuf(stdout, NULL, _IONBF, 0);

  enable_dwt_cycle_count();

  configASSERT(benchmark_queue = xQueueCreate(BENCHMARK_N, sizeof(uint32_t)));
  configASSERT(bench_semphr = xSemaphoreCreateCounting(BENCHMARK_N, 0));
  configASSERT((printf_semphr = xSemaphoreCreateMutex()));

#ifdef USE_UART_DMA
  task_uart_dma_init();
#endif

  for (size_t i = 0; i < BENCHMARK_N; ++i) {
    xTaskCreate(run_benchmark, "benchmark", configMINIMAL_STACK_SIZE, NULL,
                osPriorityNormal, NULL);
  }
  xTaskCreate(print_benchmark, "print_bench", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, NULL);

  // printf("kernel start\n");
  // printf("SystemCoreClock: %u\n", SystemCoreClock);

  osKernelStart();
}
}
