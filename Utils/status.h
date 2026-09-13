#ifndef UTILS_STATUS_H
#define UTILS_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file status.h 
 * @brief Provides a set of standardized statuses that can be returned by any function that needs to report errors. 
 * This is done so that all layers(such as CAN, ISOTP, UDS...) can communicate with each other. 
 **/ 
typedef enum Status{
    STATUS_OK = 0,
    //Expected during polling. Not an error and must not be logged.
    STATUS_NO_DATA,
    STATUS_TIMEOUT,
    STATUS_BUSY,
    STATUS_OVERFLOW,
    STATUS_INVALID_ARG,
    STATUS_INVALID_RESPONSE,
    STATUS_PROTOCOL_ERROR,
    STATUS_HW_FAULT,
    STATUS_NOT_FOUND,
}Status;

/**
 * @brief Function that converts a status to a pointer to its string equivalent stored statically in memory. 
 * Normally this is used to print statuses over UART.
 * @param[in] status A status variable of the Status enum class to be converted into a string. 
 * @retval [const char *] that points to the string equivalent of status. This string is statically stored in memory. 
 **/ 
const char *StatustoString(Status status);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_STATUS_H */
