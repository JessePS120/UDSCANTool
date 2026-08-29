#include "uart.h"
#include "cmsis_gcc.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"
#include "interrupt.h"
#include "led.h"
#include "timer.h" 
#include <stdint.h>
#include <stdbool.h>
#include <string.h> 

#define UART_TX_TIMEOUT_MS 100

static volatile uint8_t rxRingBuf[UARTRingBufMaxSize];
static volatile uint8_t rxRingBufHead = 0;
static volatile uint8_t rxRingBufTail = 0;
static volatile uint8_t rxRingBufSize = 0;

typedef enum UARTERROR{
    BSP_COM_INIT, 
    HAL_UART_RECEIVE_IT, 
    HAL_UART_TRANSMIT, 
}UARTERROR; 

static const uint32_t errorDelaysMs[] = {
    [BSP_COM_INIT]     = 1000,
    [HAL_UART_RECEIVE_IT] = 2000,
    [HAL_UART_TRANSMIT]    = 3000,
};

static void LEDBlinkError(uint32_t delayMs){
    disableInterrupts();
    while(1){ 
        toggleLED(); 
        delayMS(delayMs);
        toggleLED(); 
        delayMS(delayMs);
    } 
}

void UARTError(UARTERROR error){
    LEDBlinkError(errorDelaysMs[error]);
}

static uint8_t rxByte = 0;

void UARTInit(void){
    COM_InitTypeDef comInit;
    comInit.BaudRate   = 115200;
    comInit.WordLength = COM_WORDLENGTH_8B;
    comInit.StopBits   = COM_STOPBITS_1;
    comInit.Parity     = COM_PARITY_NONE;
    comInit.HwFlowCtl  = COM_HWCONTROL_NONE;

    if (BSP_COM_Init(COM1, &comInit) != BSP_ERROR_NONE){
        UARTError(BSP_COM_INIT);
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
        if (rxRingBufSize < UARTRingBufMaxSize){
            rxRingBuf[rxRingBufTail] = rxByte;
            rxRingBufTail = (rxRingBufTail + 1) % UARTRingBufMaxSize;
            rxRingBufSize++;
        }
        if (HAL_UART_Receive_IT(&hcom_uart[COM1], &rxByte, 1) != HAL_OK){ 
            UARTError(HAL_UART_RECEIVE_IT); 
        }
    }
}

void UARTSend(const char *data, uint16_t len, bool isTerminated){
    if(isTerminated){ 
        len = (uint16_t)strlen(data); 
    }
    if (HAL_UART_Transmit(&hcom_uart[COM1], (const uint8_t *)data, len, UART_TX_TIMEOUT_MS) != HAL_OK){
        UARTError(HAL_UART_TRANSMIT);
    }
}

bool UARTReceive(char *byte){
    if (rxRingBufSize == 0U){
        //No bytes in the ring buffer.
        return false;
    }
    __disable_irq();
    *byte = rxRingBuf[rxRingBufHead];
    rxRingBufHead = (rxRingBufHead + 1) % UARTRingBufMaxSize;
    rxRingBufSize--;
    __enable_irq();
    return true;
}

uint8_t UARTReceiveBuffer(char *buf){
    uint8_t count;

    __disable_irq();
    count = rxRingBufSize;
    if (count > 0U){
        uint8_t firstChunkLen = UARTRingBufMaxSize - rxRingBufHead;
        if (firstChunkLen > count){
            firstChunkLen = count;
        }
        //Safe because we have disabled interrutps. 
        memcpy(buf, (const void *)&rxRingBuf[rxRingBufHead], firstChunkLen);
        if (count > firstChunkLen){
            //Ring buffer wrapped, copy the remainder from the front.
            //Safe because we have disabled interrutps.
            memcpy(buf + firstChunkLen, (const void *)&rxRingBuf[0], count - firstChunkLen);
        }
        rxRingBufHead = 0; 
        rxRingBufTail = 0; 
        rxRingBufSize = 0; 
    }
    __enable_irq();

    return count;
}
