#include "isotp_user.h" 
#include "isotp_defines.h" 
#include "can.h" 
#include "timer.h" 
#include "status.h"
#include "log.h"

#define ISOTP_LOG_TAG "ISOTP"

//The library reports its own protocol faults through here. They are warnings rather than errors
//because the transport recovers on its own, and the caller still sees the failure as a return code.
void isotp_user_debug(const char* message, ...){ 
#if LOG_LEVEL >= LOG_LEVEL_WARN
    va_list args;
    va_start(args, message);
    logWriteV(LOG_LEVEL_WARN, ISOTP_LOG_TAG, message, args);
    va_end(args);
#else
    (void)message;
#endif
}

//Must convert system-wide status codes(returned from CANSend) to those used by isotp-c functions. 
//This is done to avoid a partial re-write of isotp-c functions. 
int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t* data, const uint8_t size
#ifdef ISO_TP_USER_SEND_CAN_FLAGS
                        , const uint8_t flags
#endif
#ifdef ISO_TP_USER_SEND_CAN_ARG
                        , void* arg
#endif
){ 
#ifdef ISO_TP_USER_SEND_CAN_FLAGS
    (void)flags;
#endif
#ifdef ISO_TP_USER_SEND_CAN_ARG
    (void)arg;
#endif
    Status status = CANSend(data, arbitration_id, size); 
    //Need to convert error codes here to avoid a re-write of isotp-c's code. 
    switch(status){
        case STATUS_OK:
            return ISOTP_RET_OK;
        case STATUS_BUSY:
            return ISOTP_RET_NOSPACE;
        case STATUS_OVERFLOW:
            return ISOTP_RET_OVERFLOW;
        case STATUS_INVALID_ARG:
            return ISOTP_RET_LENGTH;
        case STATUS_TIMEOUT:
            return ISOTP_RET_TIMEOUT;
        default:
            return ISOTP_RET_ERROR;
    }
}

uint32_t isotp_user_get_us(void){ 
    return GetTickMS() * 1000; 
}
