/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "thread.h"

void HAL_Delay(__IO uint32_t Delay)
{
  uint32_t tickstart = 0U;
  tickstart = HAL_GetTick();
  while((HAL_GetTick() - tickstart) < Delay)
  {
	  thread_yield();
  }
}

void in_poll_loop(){
	thread_yield();
}
