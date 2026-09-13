#ifndef APP_UART_BUF_H
#define APP_UART_BUF_H
#include <stdint.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uart_buf.h 
 * @brief Utility functions built on uart.h for reading from UART. 
 * List of macros:
 * UART_BUF_WAIT_FOREVER -> Pass as timeoutMs to UARTBufReadLine to block until a line arrives ignoring elapsed time.
**/

#define UART_BUF_WAIT_FOREVER 0xFFFFFFFFU

/**
 * @brief Function that blocks forever until a new line arrives over UART. Built off of UARTBufReadLine. 
 * @param[in, out] buf a char * that will contain the line received over UART. Note: memcpy() is used to move data into buf. 
 * @param[in] bufSize a uint32_t that represents the size of the buf. 
 * @param[in, out] outlen a uint32_t * that represents the length of the line written to buf. 
 * @retval Status enum that represents the success of the polling operation. See status.h for details. 
 **/ 
Status UARTBufPoll(char *buf, uint32_t bufSize, uint32_t *outLen); 

/**
 * @brief Function that blocks for a specified amount of time until a line is received over UART. 
 * @param[in, out] buf a char * that will contain the line received over UART. Note: memcpy() is used to move data into buf. 
 * @param[in] bufSize a uint32_t that represents the size of the buf. 
 * @param[in] timeoutMs a uint32_t that represents the amount of time to wait for a line to be received. 
 * @param[in, out] outlen a uint32_t * that represents the length of the line written to buf. 
 * @retval Status enum that represents the success of the polling operation. See status.h for details. 
 **/ 
Status UARTBufReadLine(char *buf, uint32_t bufSize, uint32_t timeoutMs, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* APP_UART_BUF_H */
