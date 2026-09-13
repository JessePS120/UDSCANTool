#include "log.h"
#include "uart.h"
#include "interrupt.h" 
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


static volatile char logRingBuf[LogRingBufMaxSize];
static volatile uint16_t logRingBufHead = 0;
static volatile uint16_t logRingBufTail = 0;
static volatile uint16_t logRingBufSize = 0;
static volatile bool logDroppedMsg = false;

static const char logLevelChars[] = { '?', 'E', 'W', 'I' };

static void logEnqueue(const char *data, uint16_t len){
    uint32_t state = enterCritical();
    if(len <= LogRingBufMaxSize - logRingBufSize){
        for(uint16_t i = 0; i < len; i++){
            logRingBuf[logRingBufTail] = data[i];
            logRingBufTail = (logRingBufTail + 1) % LogRingBufMaxSize;
        }
        logRingBufSize += len;
    }
    else{
        //Drop the whole line rather than logging a truncated one.
        logDroppedMsg = true;
    }
    exitCritical(state);
}

void logWrite(uint8_t level, const char *tag, const char *fmt, ...){ 
    va_list args; 
    va_start(args, fmt); 
    logWriteV(level, tag, fmt, args); 
    va_end(args); 
}

void logWriteV(uint8_t level, const char *tag, const char *fmt, va_list args){
    char msg[LogMaxMessageSize];

    int written = snprintf(msg, sizeof(msg), "[%c][%s] ", logLevelChars[level < (sizeof(logLevelChars) / sizeof(logLevelChars[0])) ? level : 0], tag);
    if(written < 0){
        return;
    }
    size_t offset = (size_t)written;
    if(offset > sizeof(msg) - 1){
        offset = sizeof(msg) - 1;
    }
    written = vsnprintf(msg + offset, sizeof(msg) - offset, fmt, args);
    if(written > 0){
        offset += (size_t)written;
        if(offset > sizeof(msg) - 1){
            offset = sizeof(msg) - 1;
        }
    }
    //Overwrite the tail if a line is too long to make room for the terminators. 
    if(offset > sizeof(msg) - 3){
        offset = sizeof(msg) - 3;
    }
    msg[offset++] = '\r';
    msg[offset++] = '\n';
    logEnqueue(msg, (uint16_t)offset);
}

void logDrain(void){
    char chunk[LogDrainChunkSize];

    while(true){
        //Copy out under a critical section so interrupts stay masked only for the copy, never
        //across the blocking UART transmit below.
        uint32_t state = enterCritical();
        uint16_t count = (logRingBufSize < sizeof(chunk)) ? logRingBufSize : (uint16_t)sizeof(chunk);
        for(uint16_t i = 0; i < count; i++){
            chunk[i] = logRingBuf[logRingBufHead];
            logRingBufHead = (logRingBufHead + 1) % LogRingBufMaxSize;
        }
        logRingBufSize -= count;
        exitCritical(state);

        if(count == 0){
            break;
        }
        //Ignoring errors here because any persistent error with uart should be handled by the cli or the client. 
        (void)UARTSend(chunk, count, false);
    }
    if(logDroppedMsg){
        logDroppedMsg = false;
        (void)UARTSend("[E][LOG] messages dropped, buffer full\r\n", 0, true);
    }
}
