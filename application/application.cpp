#include "application.hpp"

#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <errno.h>
#include <stdio.h>
#include <task.h>
#include <unistd.h>
#include <usart.h>

extern "C" {

// redirect stdout stdout/stderr to uart
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

void func(void) {
  HAL_Delay(500);
}

void task1(void*) {
  while (true) {
    puts("hello world");
    HAL_Delay(1000);
  }
}

void application_start() {
  const osThreadAttr_t attr = {
      .name = "defaultTask",
      .stack_size = 128 * 4,
      .priority = osPriorityNormal,
  };
  osThreadNew(task1, NULL, &attr);
  osKernelStart();
}
}
