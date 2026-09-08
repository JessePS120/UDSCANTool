#include "uart_buf.h"
#include "uart.h"
#include "timer.h"

#define UART_BUF_POLL_MS 100U 

uint8_t UARTBufPoll(char* buf, uint8_t bufSize){ 
    uint8_t UARTBytes = 0; 
    do{
        UARTBytes = UARTBufReadLine(buf, bufSize, UART_BUF_WAIT_FOREVER);
    } while(UARTBytes == 0U);
    return UARTBytes; 
}

uint8_t UARTBufReadLine(char *buf, uint8_t bufSize, uint32_t timeoutMs){
    char chunk[UARTRingBufMaxSize];
    uint8_t len = 0;
    uint32_t deadline = GetTickMS() + timeoutMs;

    while(timeoutMs == UART_BUF_WAIT_FOREVER || GetTickMS() < deadline){
        uint8_t chunkLen = UARTReceiveBuffer(chunk);
        for(uint8_t i = 0; i < chunkLen; i++){
            if(chunk[i] == '\r' || chunk[i] == '\n'){
                buf[len] = '\0';
                return len;
            }
            if(len < bufSize - 1U){
                buf[len++] = chunk[i];
            }
            //Bytes beyond bufSize are dropped and the line still ends correctly once '\r'/'\n' arrives.
        }
        delayMS(UART_BUF_POLL_MS);
    }
    return 0;
}
