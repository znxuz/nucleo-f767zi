#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <queue.h>
#include <semphr.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string_view>

extern "C" {
// int _write(int file, char* ptr, int len) {
//   HAL_StatusTypeDef hstatus;
//
//   if (file == STDOUT_FILENO || file == STDERR_FILENO) {
//     hstatus = HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
//
//     if (hstatus == HAL_OK)
//       return len;
//     else
//       return EIO;
//   }
//   errno = EBADF;
//   return -1;
// }

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
SemaphoreHandle_t xSemaphore;

void benchmark_printf(void*) {
  auto start = DWT->CYCCNT;

  for (size_t i = 0; i < 1000; ++i) {
    do {
      // must check the return value and retry like this, because it could fail
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
        printf("%s\n", lorem);
        xSemaphoreGive(xSemaphore);
        break;
      }
    } while (true);
  }

  auto end = DWT->CYCCNT;
  uint32_t time_us =
      static_cast<double>(end - start) / SystemCoreClock * 1000 * 1000;

  vTaskDelay(1000);
  printf("time us elapsed: %u\n", time_us);
  vTaskDelete(NULL);
}

void task2(void*) {
  std::string_view task2_name = "t2_fn";
  while (true) {
    printf("%s\n", task2_name.data());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void application_start() {
  enable_dwt_cycle_count();

  task_uart_dma_init();
  configASSERT((xSemaphore = xSemaphoreCreateMutex()));
  xTaskCreate(benchmark_printf, "bench1", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, NULL);
  xTaskCreate(benchmark_printf, "bench2", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, NULL);
  xTaskCreate(benchmark_printf, "bench3", configMINIMAL_STACK_SIZE, NULL,
              osPriorityNormal, NULL);
  // xTaskCreate(task2, "task2", configMINIMAL_STACK_SIZE, NULL,
  // osPriorityNormal,
  //             NULL);

  // printf("kernel start\n");
  // printf("SystemCoreClock: %u\n", SystemCoreClock);

  osKernelStart();
}
}
