#include "uds.h"
#include "uds_cfg.h"
#include "isotp.h"
#include <string.h>

//Using switch here to save on memory. 
const char *UDS_NRCtoString(UDS_NRC nrc){
    switch(nrc){
        case UDS_NRC_GENERAL_REJECT:                            return "UDS_NRC_GENERAL_REJECT";
        case UDS_NRC_SERVICE_NOT_SUPPORTED:                     return "UDS_NRC_SERVICE_NOT_SUPPORTED";
        case UDS_NRC_SUBFUNCTION_NOT_SUPPORTED:                 return "UDS_NRC_SUBFUNCTION_NOT_SUPPORTED";
        case UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT:return "UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT";
        case UDS_NRC_RESPONSE_TOO_LONG:                         return "UDS_NRC_RESPONSE_TOO_LONG";
        case UDS_NRC_BUSY_REPEAT_REQUEST:                       return "UDS_NRC_BUSY_REPEAT_REQUEST";
        case UDS_NRC_CONDITIONS_NOT_CORRECT:                    return "UDS_NRC_CONDITIONS_NOT_CORRECT";
        case UDS_NRC_REQUEST_SEQUENCE_ERROR:                    return "UDS_NRC_REQUEST_SEQUENCE_ERROR";
        case UDS_NRC_REQUEST_OUT_OF_RANGE:                      return "UDS_NRC_REQUEST_OUT_OF_RANGE";
        case UDS_NRC_RESPONSE_PENDING:                          return "UDS_NRC_RESPONSE_PENDING";
        case UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_SESSION:      return "UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_SESSION";
        case UDS_NRC_SERVICE_NOT_SUPPORTED_IN_SESSION:          return "UDS_NRC_SERVICE_NOT_SUPPORTED_IN_SESSION";
    }
    return "UDS_NRC_UNKNOWN";
}

static const char *const udsStatusStrings[] = {
    [UDS_STATUS_OK]                = "UDS_STATUS_OK",
    [UDS_STATUS_NEGATIVE_RESPONSE] = "UDS_STATUS_NEGATIVE_RESPONSE",
    [UDS_STATUS_TIMEOUT]           = "UDS_STATUS_TIMEOUT",
    [UDS_STATUS_TRANSPORT_ERROR]   = "UDS_STATUS_TRANSPORT_ERROR",
    [UDS_STATUS_INVALID_RESPONSE]  = "UDS_STATUS_INVALID_RESPONSE",
    [UDS_STATUS_REQUEST_TOO_LARGE] = "UDS_STATUS_REQUEST_TOO_LARGE",
};

const char *UdsStatustoString(UdsStatus status){
    uint8_t index = (uint8_t)status;
    if(index >= (sizeof(udsStatusStrings) / sizeof(udsStatusStrings[0])) || udsStatusStrings[index] == NULL){
        return "UDS_STATUS_UNKNOWN";
    }
    return udsStatusStrings[index];
}

UdsResult sendUdsRequest(uint8_t sid, uint8_t *data, uint16_t len, UdsResponse *response){
    response->sid = sid;
    response->nrc = 0;
    response->data = NULL;
    response->len = 0;

    if(len > ISOTP_MAX_MESSAGE_SIZE - 1){
        return (UdsResult){ UDS_STATUS_REQUEST_TOO_LARGE, ISOTP_STATUS_OK };
    }

    uint8_t pdu[ISOTP_MAX_MESSAGE_SIZE];
    pdu[0] = sid;
    if(len > 0){
        memcpy(&pdu[1], data, len);
    }
    ISOTPSTATUS sendStatus = sendFrame(pdu, len + 1);
    if(sendStatus != ISOTP_STATUS_OK){
        return (UdsResult){ UDS_STATUS_TRANSPORT_ERROR, sendStatus };
    }

    IsoTpMsg msg = {0};
    uint32_t timeoutMs = UDS_P2_CLIENT_MS;
    while(1){
        ISOTPSTATUS recvStatus = getFrames(&msg, timeoutMs);
        if(recvStatus == ISOTP_NO_FRAME_RECEIVED){
            //No frame arrived within the timeout window.
            return (UdsResult){ UDS_STATUS_TIMEOUT, recvStatus };
        }
        if(recvStatus != ISOTP_STATUS_OK){
            return (UdsResult){ UDS_STATUS_TRANSPORT_ERROR, recvStatus };
        }
        if(msg.data[0] == UDS_NEGATIVE_RESPONSE_SID){
            //Negative response should include rejected SID.
            if(msg.len < 3 || msg.data[1] != sid){
                return (UdsResult){ UDS_STATUS_INVALID_RESPONSE, ISOTP_STATUS_OK };
            }
            uint8_t nrc = msg.data[2];
            if(nrc == UDS_NRC_RESPONSE_PENDING){
                //Server needs more time to process the request, keep waiting up to P2*.
                timeoutMs = UDS_P2_STAR_CLIENT_MS;
                continue;
            }
            response->nrc = nrc;
            return (UdsResult){ UDS_STATUS_NEGATIVE_RESPONSE, ISOTP_STATUS_OK };
        }
        if(msg.data[0] == (uint8_t)(sid + UDS_POSITIVE_RESPONSE_OFFSET)){
            response->data = &msg.data[1];
            response->len = msg.len - 1;
            return (UdsResult){ UDS_STATUS_OK, ISOTP_STATUS_OK };
        }
        //Response SID doesn't match what we sent.
        return (UdsResult){ UDS_STATUS_INVALID_RESPONSE, ISOTP_STATUS_OK };
    }
}

UdsResult udsTesterPresent(UdsResponse *response){
    uint8_t subFunction = 0x00;
    return sendUdsRequest(UDS_SID_TESTER_PRESENT, &subFunction, 1, response);
}

UdsResult udsRequestVin(UdsResponse *response){
    uint8_t pdu[2] = { (uint8_t)(UDS_DID_VIN >> 8), (uint8_t)(UDS_DID_VIN & 0xFF) };
    return sendUdsRequest(UDS_SID_READ_DATA_BY_IDENTIFIER, pdu, sizeof(pdu), response);
}
