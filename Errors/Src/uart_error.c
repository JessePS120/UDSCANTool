#include "uart_error.h"
#include "interrupt.h"
#include "timer.h"

void LEDBlinkError(uint32_t delayMS){
    disableInterrupts(); 
    toggleLED(); 
    delayMS(delayMS);
    toggleLED(); 
    delayMS(delayMS);
}

static const uint32_t errorDelaysMs[] = {
    [BSP_COM_INIT]     = 1000,
    [HAL_UART_RECEIVE_IT] = 2000,
    [HAL_UART_TRANSMIT]    = 3000,
};

void UARTError(UARTERROR error){
    LEDBlinkError(errorDelaysMs[error]);
}
