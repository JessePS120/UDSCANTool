#ifndef ERRORS_UART_ERROR_H
#define ERRORS_UART_ERROR_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UARTERROR{
    BSP_COM_INIT, 
    HAL_UART_RECEIVE_IT, 
    HAL_UART_TRANSMIT, 
}UARTERROR; 

void LEDInit(void);

void LEDBlinkError(uint32_t delayMS);

void UARTError(UARTERROR error); 

#ifdef __cplusplus
}
#endif

#endif /* ERRORS_UART_ERROR_H */
