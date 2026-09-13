#include "isotp_port.h" 
#include "can.h"
#include "isotp_defines.h"
#include "isotp.h" 
#include "timer.h"
#include "log.h"
#include <stdbool.h>
#include <stdint.h> 

#define ISOTP_PORT_LOG_TAG "ISOTP"

static uint8_t isotpSendBuf[ISOTP_SEND_BUF_SIZE]; 
static uint8_t isotpRecBuf[ISOTP_REC_BUF_SIZE]; 
static IsoTpLink isotpLink = {0}; 
static uint32_t isotpTxID = 0; 
//Filter using this ID. The sender ID is stored in the link so no need for a static variable. 
static uint32_t isotpRxID = 0; 
static bool isotpReady = false; 

//Converter for the isotp-c status codes. 
static Status isotpToStatus(int ret){ 
    switch(ret){
        case ISOTP_RET_OK:         return STATUS_OK;
        case ISOTP_RET_NO_DATA:    return STATUS_NO_DATA;
        case ISOTP_RET_INPROGRESS: return STATUS_BUSY;
        case ISOTP_RET_NOSPACE:    return STATUS_BUSY;
        case ISOTP_RET_OVERFLOW:   return STATUS_OVERFLOW;
        case ISOTP_RET_LENGTH:     return STATUS_INVALID_ARG;
        case ISOTP_RET_TIMEOUT:    return STATUS_TIMEOUT;
        case ISOTP_RET_WRONG_SN:   return STATUS_PROTOCOL_ERROR;
        default:                   return STATUS_PROTOCOL_ERROR;
    }
}

//This function exists in case we need to reset the link(typically due to a failure). 
static void isotpResetLink(void){ 
    isotp_init_link(&isotpLink, isotpTxID, isotpSendBuf, ISOTP_SEND_BUF_SIZE, isotpRecBuf, ISOTP_REC_BUF_SIZE); 
}

Status isotpInit(uint32_t txID, uint32_t rxID){ 
    isotpTxID = txID; 
    isotpRxID = rxID; 
    isotpResetLink(); 
    isotpReady = true; 
    return STATUS_OK; 
} 

Status isotpPump(void){ 
    if(!isotpReady){
        LOG_ERROR(ISOTP_PORT_LOG_TAG, "used before isotpInit()");
        return STATUS_NOT_FOUND;
    } 
    CANMsg msg = {0};  
    Status CANStatus = CANReceive(&msg); 
    if(CANStatus == STATUS_OK && msg.id == isotpRxID){
        isotp_on_can_message(&isotpLink, msg.data, msg.len); 
    }
    isotp_poll(&isotpLink);
    return CANStatus; 
}

Status isotpSend(const uint8_t *msg, uint32_t len, uint32_t timeoutMs){
    if(msg == NULL || len == 0){
        return STATUS_INVALID_ARG;
    }
    if(!isotpReady){
        LOG_ERROR(ISOTP_PORT_LOG_TAG, "used before isotpInit()");
        return STATUS_NOT_FOUND;
    }

    int ret = isotp_send(&isotpLink, msg, len);
    if(ret != ISOTP_RET_OK){
        Status status = isotpToStatus(ret);
        LOG_ERROR(ISOTP_PORT_LOG_TAG, "send rejected: %s", StatustoString(status));
        return status;
    }

    uint32_t start = GetTickMS();
    while(isotpLink.send_status == ISOTP_SEND_STATUS_INPROGRESS){
        if((GetTickMS() - start) >= timeoutMs){
            LOG_WARN(ISOTP_PORT_LOG_TAG, "send timed out after %lu ms", (unsigned long)timeoutMs);
            isotpResetLink();
            return STATUS_TIMEOUT;
        }
        //Calling this to receive the flow control frame. 
        (void)isotpPump();
    }
    if(isotpLink.send_status == ISOTP_SEND_STATUS_ERROR){
        LOG_ERROR(ISOTP_PORT_LOG_TAG, "send failed, protocol result %ld", (long)isotpLink.send_protocol_result);
        //Reset the link and hopefully recover next time. 
        isotpResetLink();
        return STATUS_PROTOCOL_ERROR;
    }
    return STATUS_OK;
}

Status isotpRec(uint8_t *buf, uint32_t bufLen, uint32_t *bytesCopied, uint32_t timeoutMs){ 
    if(buf == NULL || bytesCopied == NULL || bufLen == 0){
        return STATUS_INVALID_ARG;
    }
    *bytesCopied = 0;
    if(!isotpReady){
        LOG_ERROR(ISOTP_PORT_LOG_TAG, "used before isotpInit()");
        return STATUS_NOT_FOUND;
    }

    uint32_t start = GetTickMS();
    while(true){ 
        //Advance the link. 
        (void)isotpPump();
        int ret = isotp_receive(&isotpLink, buf, bufLen, bytesCopied); 
        if(ret == ISOTP_RET_OK){
            return STATUS_OK;
        }
        //Catch any error returned ny isotpToStatus. 
        if(ret != ISOTP_RET_NO_DATA){
            Status status = isotpToStatus(ret);
            LOG_ERROR(ISOTP_PORT_LOG_TAG, "receive failed: %s", StatustoString(status));
            return status;
        }
        if((GetTickMS() - start) >= timeoutMs){
            return STATUS_TIMEOUT;
        }
    }
}  
