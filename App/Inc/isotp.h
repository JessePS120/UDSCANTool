#ifndef APP_ISOTP_H
#define APP_ISOTP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ISOTP_MAX_MESSAGE_SIZE 16 

typedef struct IsoTpMsg {
    uint32_t id;
    uint8_t *data;
    uint16_t len;
} IsoTpMsg;

typedef enum ISOTPSTATUS{
    ISOTP_N_CR_TIMEOUT,
    ISOTP_INVALID_CONSECUTIVE_FRAME,
    ISOTP_SEQUENCE_NUMBER_MISMATCH,
    ISOTP_NO_FRAME_RECEIVED, 
    ISOTP_INVALID_FIRST_FRAME_LENGTH,
    ISOTP_INVALID_FRAME_TYPE,
    ISOTP_N_BS_TIMEOUT,
    ISOTP_INVALID_FLOW_CONTROL_FRAME,
    ISOTP_FLOW_CONTROL_FRAME_MAX_EXCEEDED,
    ISOTP_FLOW_CONTROL_OVERFLOW_ABORT,
    ISOTP_INVALID_FLOW_CONTROL_STATUS,
    ISOTP_STATUS_OK, 
}ISOTPSTATUS;

const char *ISOTPSTATUStoString(ISOTPSTATUS status);

ISOTPSTATUS getFrames(IsoTpMsg *msg, uint32_t timeoutMs);

ISOTPSTATUS sendFrame(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* APP_ISOTP_H */
