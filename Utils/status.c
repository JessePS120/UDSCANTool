#include "status.h"
#include <stddef.h>
#include <stdint.h>

static const char *const statusStrings[] = {
    [STATUS_OK]               = "STATUS_OK",
    [STATUS_NO_DATA]          = "STATUS_NO_DATA",
    [STATUS_TIMEOUT]          = "STATUS_TIMEOUT",
    [STATUS_BUSY]             = "STATUS_BUSY",
    [STATUS_OVERFLOW]         = "STATUS_OVERFLOW",
    [STATUS_INVALID_ARG]      = "STATUS_INVALID_ARG",
    [STATUS_INVALID_RESPONSE] = "STATUS_INVALID_RESPONSE",
    [STATUS_PROTOCOL_ERROR]   = "STATUS_PROTOCOL_ERROR",
    [STATUS_HW_FAULT]         = "STATUS_HW_FAULT",
    [STATUS_NOT_FOUND]        = "STATUS_NOT_FOUND",
};

const char *StatustoString(Status status){
    uint8_t index = (uint8_t)status;
    if(index >= (sizeof(statusStrings) / sizeof(statusStrings[0]))){
        return "STATUS_UNKNOWN";
    }
    return statusStrings[index];
}
