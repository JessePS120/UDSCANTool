#ifndef APP_UART_H
#define APP_UART_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UARTRingBufMaxSize 64
//Avoid changing this macro name unless the change is reflected in 
//serial.py. 
#define UART_BAUD_RATE 115200 

void UARTInit(void);

void UARTSend(const char *data, uint16_t len, bool isTerminated);

bool UARTReceive(char *byte);

void UARTFlushBuf(void); 

uint8_t UARTReceiveBuffer(char *buf);

#ifdef __cplusplus
}
#endif

#endif /* APP_UART_H */
