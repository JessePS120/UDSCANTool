#include "led.h" 
#include "status.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"

void toggleLED(void){ 
    BSP_LED_Toggle(LED_GREEN);
}

//Not logged here: the LED is what reports a UART fault, so it is brought up before logging exists.
Status LEDInit(void){
    if(BSP_LED_Init(LED_GREEN) != BSP_ERROR_NONE){
        return STATUS_HW_FAULT;
    }
    return STATUS_OK;
}
