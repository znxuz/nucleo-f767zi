#include <cmsis_os2.h>
#include <tim.h>

#include <cstdio>

extern "C" {
void task_record_init();
void task_button_init();

void benchmark_streambuf();

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

void application_start() {
  enable_dwt_cycle_count();
  setvbuf(stdout, NULL, _IONBF, 0);

  benchmark_streambuf();

  osKernelStart();
}
}
