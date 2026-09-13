#include "uart.h"
#include "status.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"
#include "interrupt.h"
#include "led.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h> 

#define UART_TX_TIMEOUT_MS 100

//Blink periods used to distinguish faults that cannot be reported over UART itself.
#define UART_PANIC_BLINK_RX_MS 2000U

static volatile uint8_t rxRingBuf[UARTRingBufMaxSize];
static volatile uint8_t rxRingBufHead = 0;
static volatile uint8_t rxRingBufTail = 0;
static volatile uint8_t rxRingBufSize = 0;

// This function is needed because entering an UART error state will cause interrupts to be disabled. 
//if this happens, we cannot use HAL_Delay(). 
static void UARTPanicDelay(uint32_t delayMs){
    //Approximate; the loop body is a few cycles, which only makes the blink slower than asked.
    for(uint32_t cycles = (SystemCoreClock / 1000U) * delayMs; cycles > 0U; cycles--){
        __NOP();
    }
}

//Blink an LED if the UART hardware fails. 
static void UARTPanic(uint32_t delayMs){
    enterCritical();
    while(1){ 
        toggleLED(); 
        UARTPanicDelay(delayMs);
    } 
}

static uint8_t rxByte = 0;

Status UARTInit(void){
    COM_InitTypeDef comInit;
    comInit.BaudRate   = UART_BAUD_RATE;
    comInit.WordLength = COM_WORDLENGTH_8B;
    comInit.StopBits   = COM_STOPBITS_1;
    comInit.Parity     = COM_PARITY_NONE;
    comInit.HwFlowCtl  = COM_HWCONTROL_NONE;

    //Not logged here: logging only works once this succeeds, so the caller decides what to do.
    if (BSP_COM_Init(COM1, &comInit) != BSP_ERROR_NONE){
        return STATUS_HW_FAULT;
    }

    //May need to change this priority later.
    HAL_NVIC_SetPriority(LPUART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    if (HAL_UART_Receive_IT(&hcom_uart[COM1], &rxByte, 1) != HAL_OK){
        return STATUS_HW_FAULT;
    }
    return STATUS_OK;
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
        //Failing to re-arm leaves the console permanently deaf and there is no path to report it.
        if (HAL_UART_Receive_IT(&hcom_uart[COM1], &rxByte, 1) != HAL_OK){ 
            UARTPanic(UART_PANIC_BLINK_RX_MS); 
        }
    }
}

Status UARTSend(const char *data, uint32_t len, bool isTerminated){
    if(data == NULL){
        return STATUS_INVALID_ARG;
    }
    if(isTerminated){ 
        len = (uint16_t)strlen(data); 
    }
    if(len == 0U){
        return STATUS_OK;
    }
    HAL_StatusTypeDef halStatus = HAL_UART_Transmit(&hcom_uart[COM1], (const uint8_t *)data, len, UART_TX_TIMEOUT_MS);
    if (halStatus == HAL_TIMEOUT){
        return STATUS_TIMEOUT;
    }
    if (halStatus == HAL_BUSY){
        return STATUS_BUSY;
    }
    if (halStatus != HAL_OK){
        return STATUS_HW_FAULT;
    }
    return STATUS_OK;
}

Status UARTReceive(char *byte){
    if (byte == NULL){
        return STATUS_INVALID_ARG;
    }
    if (rxRingBufSize == 0U){
        //No bytes in the ring buffer.
        return STATUS_NO_DATA;
    }
    uint32_t state = enterCritical();
    *byte = (char)rxRingBuf[rxRingBufHead];
    rxRingBufHead = (rxRingBufHead + 1) % UARTRingBufMaxSize;
    rxRingBufSize--;
    exitCritical(state);
    return STATUS_OK;
}

void UARTFlushBuf(void){
    uint32_t state = enterCritical();
    rxRingBufHead = 0; 
    rxRingBufTail = 0; 
    rxRingBufSize = 0; 
    exitCritical(state);
}

Status UARTReceiveBuffer(char *buf, uint32_t *outLen){
    if (buf == NULL || outLen == NULL){
        return STATUS_INVALID_ARG;
    }
    *outLen = 0;

    uint32_t state = enterCritical();
    uint32_t count = rxRingBufSize;
    if (count > 0U){
        uint32_t firstChunkLen = UARTRingBufMaxSize - rxRingBufHead;
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
    exitCritical(state);

    *outLen = count;
    return (count > 0U) ? STATUS_OK : STATUS_NO_DATA;
}
