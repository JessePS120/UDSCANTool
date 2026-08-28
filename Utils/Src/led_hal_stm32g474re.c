#include "led.h" 
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"

void toggleLED(void){ 
    BSP_LED_Toggle(LED_GREEN);
}

void LEDInit(void){
    BSP_LED_Init(LED_GREEN);
}