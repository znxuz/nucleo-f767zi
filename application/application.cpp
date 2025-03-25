#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <main.h>
#include <printf.h>
#include <task.h>
#include <tim.h>

#include <threadsafe_sink.hpp>
#include <utility>

#include "usart.h"

using namespace freertos;

extern "C" {
void benchmark_tsink();

volatile unsigned long ulHighFrequencyTimerTicks;

void configureTimerForRunTimeStats(void) {
  ulHighFrequencyTimerTicks = 0;
  HAL_TIM_Base_Start_IT(&htim13);
}

unsigned long getRunTimeCounterValue(void) { return ulHighFrequencyTimerTicks; }

static void enable_dwt_cycle_count() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->LAR = 0xC5ACCE55;  // software unlock
  DWT->CYCCNT = 1;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  static constexpr uint8_t DEBOUNCE_TIME_MS = 50;
  static volatile uint32_t last_interrupt_time = 0;

  if (GPIO_Pin != USER_Btn_Pin) return;

  uint32_t current_time = HAL_GetTick();
  if (current_time - std::exchange(last_interrupt_time, current_time) >
      DEBOUNCE_TIME_MS) {
    // TODO
    // static BaseType_t xHigherPriorityTaskWoken;
    // vTaskNotifyGiveFromISR(take_task_hdl, &xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance != huart3.Instance) return;
  tsink::consume_complete<freertos::tsink::CALLSITE::ISR>();
}

void application_start() {
  enable_dwt_cycle_count();

  [[maybe_unused]] auto tsink_consume_dma = [](const uint8_t* buf,
                                               size_t size) static {
    auto flush_cache_aligned = [](uintptr_t addr, size_t size) static {
      constexpr auto align_addr = [](uintptr_t addr) { return addr & ~0x1F; };
      constexpr auto align_size = [](uintptr_t addr, size_t size) {
        return size + ((addr) & 0x1F);
      };

      SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(align_addr(addr)),
                              align_size(addr, size));
    };

    flush_cache_aligned(reinterpret_cast<uintptr_t>(buf), size);
    HAL_UART_Transmit_DMA(&huart3, buf, size);
  };

  [[maybe_unused]] auto tsink_consume = [](const uint8_t* buf,
                                           size_t size) static {
    HAL_UART_Transmit(&huart3, buf, size, HAL_MAX_DELAY);
    tsink::consume_complete<tsink::CALLSITE::NON_ISR>();
  };

  tsink::init(tsink_consume_dma, osPriorityAboveNormal);
  // tsink::init(tsink_consume, osPriorityAboveNormal);
  benchmark_tsink();

  osKernelStart();
}
}
