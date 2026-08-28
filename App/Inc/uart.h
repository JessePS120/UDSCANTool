#ifndef APP_UART_H
#define APP_UART_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void UARTInit(void);

void UARTSend(uint8_t *data, uint16_t len, bool isTerminated);

bool UARTReceive(uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif /* APP_UART_H */
