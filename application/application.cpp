#include "application.hpp"

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <errno.h>
#include <stdio.h>
#include <stm32f767xx.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

#include "freertos_utils.hpp"

#define LOG_CYCCNT(func)                                                 \
  do {                                                                   \
    uint32_t timestamp;                                                  \
    timestamp = DWT->CYCCNT;                                             \
    func;                                                                \
    timestamp = DWT->CYCCNT - timestamp;                                 \
    printf("%.07f\n", static_cast<double>(timestamp) / SystemCoreClock); \
  } while (0)

// redirect stdout stdout/stderr to uart
extern "C" int _write(int file, char* ptr, int len) {
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

void enable_dwt_cycle_count() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;  // software unlock
  DWT->CYCCNT = 1;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

class Thread : public freertos::AbstractThread {
 public:
  using freertos::AbstractThread::AbstractThread;

 private:
  void run(void* arg) final {
    while (true) {
    puts("measuring");
      LOG_CYCCNT(vTaskDelay(pdMS_TO_TICKS(10000)));
      HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
    }
  }
};

void application_start() {
  enable_dwt_cycle_count();

  auto t = Thread{"thread1", 128 * 4, osPriorityNormal, nullptr, true};

  osKernelStart();
}
