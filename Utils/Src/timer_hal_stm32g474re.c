#include "timer.h" 
#include "stm32g4xx_hal.h"
#include <stdint.h> 

uint32_t GetTickMs(void){
    return HAL_GetTick();
}

void delayMS(uint32_t delay){ 
    HAL_Delay(delay); 
}