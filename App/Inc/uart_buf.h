#ifndef APP_UART_BUF_H
#define APP_UART_BUF_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//Pass as timeoutMs to UARTBufReadLine to block until a line arrives, ignoring elapsed time.
#define UART_BUF_WAIT_FOREVER 0xFFFFFFFFU

uint8_t UARTBufPoll(char* buf, uint8_t bufSize); 

uint8_t UARTBufReadLine(char *buf, uint8_t bufSize, uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_UART_BUF_H */
