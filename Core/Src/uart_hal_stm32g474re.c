#include "uart.h"
#include "uart_error.h" 
#include "cmsis_gcc.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h> 

#define rxRingBufMaxSize 64
#define UART_TX_TIMEOUT_MS 100

static volatile uint8_t rxRingBuf[rxRingBufMaxSize];
static volatile uint8_t rxRingBufHead = 0;
static volatile uint8_t rxRingBufTail = 0;
static volatile uint8_t rxRingBufSize = 0;

static uint8_t rxByte = 0;

void UARTInit(void){
    COM_InitTypeDef comInit;
    comInit.BaudRate   = 115200;
    comInit.WordLength = COM_WORDLENGTH_8B;
    comInit.StopBits   = COM_STOPBITS_1;
    comInit.Parity     = COM_PARITY_NONE;
    comInit.HwFlowCtl  = COM_HWCONTROL_NONE;

    if (BSP_COM_Init(COM1, &comInit) != BSP_ERROR_NONE){
        UART_Error(BSP_COM_INIT);
    }

    //May need to change this priority later.
    HAL_NVIC_SetPriority(LPUART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
}

void LPUART1_IRQHandler(void){
    HAL_UART_IRQHandler(&hcom_uart[COM1]);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if (huart->Instance == LPUART1){
        //Drop the byte on overflow rather than blocking in an ISR.
        if (rxRingBufSize < rxRingBufMaxSize){
            rxRingBuf[rxRingBufTail] = rxByte;
            rxRingBufTail = (rxRingBufTail + 1) % rxRingBufMaxSize;
            rxRingBufSize++;
        }
        if (HAL_UART_Receive_IT(&hcom_uart[COM1], &rxByte, 1) != HAL_OK){ 
            UARTError(HAL_UART_RECEIVE_IT); 
        }
    }
}

void UARTSend(uint8_t *data, uint16_t len, bool isTerminated){
    if(isTerminated){ 
        len = strlen(data); 
    }
    if (HAL_UART_Transmit(&hcom_uart[COM1], data, len, UART_TX_TIMEOUT_MS) != HAL_OK){
        UARTError(HAL_UART_TRANSMIT);
    }
}

bool UARTReceive(uint8_t *byte){
    if (rxRingBufSize == 0U){
        //No bytes in the ring buffer.
        return false;
    }
    __disable_irq();
    *byte = rxRingBuf[rxRingBufHead];
    rxRingBufHead = (rxRingBufHead + 1) % rxRingBufMaxSize;
    rxRingBufSize--;
    __enable_irq();
    return true;
}
