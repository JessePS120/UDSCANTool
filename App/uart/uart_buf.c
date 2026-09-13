#include "uart_buf.h"
#include "uart.h"
#include "timer.h"
#include "status.h"
#include <string.h> 
#include <stddef.h>

#define UART_BUF_POLL_MS 100U 

Status UARTBufPoll(char *buf, uint32_t bufSize, uint32_t *outLen){ 
    Status status;
    do{
        status = UARTBufReadLine(buf, bufSize, UART_BUF_WAIT_FOREVER, outLen);
        //Propagate this status back up to the caller as it will constantly be returned. 
        if(status == STATUS_INVALID_ARG){
            return status;
        }
    //An empty line is not a command, so keep waiting for one with characters.
    } while(status != STATUS_OK || *outLen == 0U);
    return STATUS_OK; 
}

Status UARTBufReadLine(char *buf, uint32_t bufSize, uint32_t timeoutMs, uint32_t *outLen){
    if(buf == NULL || outLen == NULL || bufSize == 0U){
        return STATUS_INVALID_ARG;
    }
    *outLen = 0;
    buf[0] = '\0';

    char chunk[UARTRingBufMaxSize];
    uint8_t len = 0;
    uint32_t deadline = GetTickMS() + timeoutMs;

    while(timeoutMs == UART_BUF_WAIT_FOREVER || GetTickMS() < deadline){
        uint8_t chunkLen = 0;
        //NOTE UARTReceiveBuffer may return "STATUS_NO_DATA" but that is ok. We can continue looping here. 
        Status status = UARTReceiveBuffer(chunk, &chunkLen);
        if(status == STATUS_INVALID_ARG){
            return status;
        }
        if(status != STATUS_NO_DATA){
            if(len + chunkLen < bufSize){
                memcpy(&buf[len], (const void *)chunk, chunkLen); 
                len += chunkLen; 
                if(chunk[chunkLen - 1U] == '\r' || chunk[chunkLen - 1U] == '\n'){
                    //Strip every trailing CR/LF. 
                    while(len > 0U && (buf[len - 1U] == '\r' || buf[len - 1U] == '\n')){
                        len--;
                    }
                    *outLen = len;
                    buf[len] = '\0';
                    return STATUS_OK; 
                }
            }
            else{ 
                //Copy whatever bytes we can. 
                memcpy(&buf[len], (const void *)chunk, bufSize - len); 
                *outLen = bufSize;  
                buf[bufSize - 1] = '\0'; 
                return STATUS_OVERFLOW; 
            } 
        } 
        delayMS(UART_BUF_POLL_MS);
    }
    return STATUS_TIMEOUT;
}
