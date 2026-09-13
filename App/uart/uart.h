#ifndef APP_UART_H
#define APP_UART_H
#include <stdint.h>
#include <stdbool.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uart.h 
 * @brief Provides portable wrappers around platform specific code to initialize and utilize UART. 
 * List of macros:
 * UARTRingBufMaxSize -> Size of the ring buffer used to store messages received from UART. Avoid changing this macro name unless the change is reflected in serial.py.
 * UART_BAUD_RATE -> Baud rate used by UART. 
**/

#define UARTRingBufMaxSize 150 
#define UART_BAUD_RATE 115200 

/**
 * @brief Function that initializes the hardware needed for UART. 
 * @retval A Status enum that represents the success of initializing the UART hardware. See status.h for details. 
 **/
Status UARTInit(void);

/**
 * @brief Function for sending a string over UART. 
 * @param[in] data a const char * pointing to the string to be send over uart. 
 * @param[in] len a uint32_t that represents the size of the string pointed to by data. 
 * @param[in] isTerminated a flag for if the string has been terminated. len is ignored if this is set to true. 
 * @retval Status enum that represents the success of sending over UART. See status.h for details. 
 **/
Status UARTSend(const char *data, uint32_t len, bool isTerminated);

/**
 * @brief Function for receiving a single byte from the UART ring buffer. 
 * @param[in] byte a char * pointing to the memory location where the byte will be placed. 
 * @retval Status enum that represents the success of sending over UART. See status.h for details. 
 **/
Status UARTReceive(char *byte);

/**
 * @brief Function for flushing the UART ring buffer. 
 **/
void UARTFlushBuf(void); 

/**
 * @brief Function for draining the entire UART ring buffer. 
 * @param[in, out] buf a char * for the buffer the UART ring buffer will be written to. IMPORTANT: The buffer must be at least UARTRingBufMaxSize bytes big. 
 * @param[in, out] outlen a uint32_t that represents the number of bytes written to buf. 
 * @retval Status enum that represents the success of copying bytes from the UART ring buffer. See status.h for details. 
 **/
Status UARTReceiveBuffer(char *buf, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* APP_UART_H */
