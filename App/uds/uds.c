#include "uds.h"
#include "uds_cfg.h"
#include "isotp_port.h"
#include "status.h" 
#include <stddef.h>
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

//One request and one response are in flight at a time, so both live here for the whole call.
//response->data points into udsRecBuf, which the next request overwrites.
static uint8_t udsSendBuf[MAX_UDS_MSG_SIZE];
static uint8_t udsRecBuf[MAX_UDS_MSG_SIZE];

static Status sendUdsRequest(uint8_t sid, const uint8_t *data, uint32_t len, UdsResponse *response){
    if(response == NULL || (len > 0 && data == NULL)){
        return STATUS_INVALID_ARG;
    }
    *response = (UdsResponse){0};
    response->sid = sid;

    //See: https://www.csselectronics.com/pages/uds-protocol-tutorial-unified-diagnostic-services
    //PCI(handled by isotp) | SID | Sub Func and data params | Padding(handled by isotp). 
    if((uint32_t)len + 1U > sizeof(udsSendBuf)){
        return STATUS_OVERFLOW; 
    }
    udsSendBuf[0] = sid;
    if(len > 0){
        memcpy(&udsSendBuf[1], data, len);
    }

    Status status = isotpSend(udsSendBuf, (uint32_t)len + 1U, UDS_REQUEST_SEND_TIMEOUT_MS);
    if(status != STATUS_OK){
        return status; 
    }

    uint32_t timeoutMs = UDS_P2_CLIENT_MS;
    while(1){
        uint32_t recLen = 0;
        status = isotpRec(udsRecBuf, sizeof(udsRecBuf), &recLen, timeoutMs);
        if(status != STATUS_OK){
            //No response arrived within the window, or the transport faulted.
            return status; 
        }
        if(recLen == 0){
            return STATUS_INVALID_RESPONSE;
        }
        if(udsRecBuf[0] == UDS_NEGATIVE_RESPONSE_SID){
            //Negative response should include rejected SID.
            if(recLen < 3 || udsRecBuf[1] != sid){
                return STATUS_INVALID_RESPONSE;
            }
            uint8_t nrc = udsRecBuf[2];
            if(nrc == UDS_NRC_RESPONSE_PENDING){
                //Server needs more time to process the request, keep waiting up to P2*.
                timeoutMs = UDS_P2_STAR_CLIENT_MS;
                continue;
            }
            response->nrc = nrc;
            return STATUS_PROTOCOL_ERROR;
        }
        if(udsRecBuf[0] == (uint8_t)(sid + UDS_POSITIVE_RESPONSE_OFFSET)){
            response->data = &udsRecBuf[1];
            response->len = (uint16_t)(recLen - 1U);
            return STATUS_OK;
        }
        //Response SID doesn't match what we sent.
        return STATUS_INVALID_RESPONSE;
    }
}

//Functions for each UDS command. 
static Status udsTesterPresent(UdsResponse *response){
    uint8_t subFunction = 0x00;
    return sendUdsRequest(UDS_SID_TESTER_PRESENT, &subFunction, 1, response);
}

static Status udsRequestVin(UdsResponse *response){
    uint8_t did[2] = { (uint8_t)(UDS_DID_VIN >> 8), (uint8_t)(UDS_DID_VIN & 0xFF) };
    return sendUdsRequest(UDS_SID_READ_DATA_BY_IDENTIFIER, did, sizeof(did), response);
}

//Generic UDS function exposed for use by the client/server. Avoid calling specific UDS commands directly due 
//to lack of error handling. 
Status sendUDSCmd(const char *cmd, UdsResponse *response){
    if(cmd == NULL || response == NULL){
        return STATUS_INVALID_ARG;
    }
    //Setting to default value.
    *response = (UdsResponse){0};

    uint32_t len = strlen(cmd);
    if(len == strlen("TESTERPRESENT") && strncmp(cmd, "TESTERPRESENT", len) == 0){
        return udsTesterPresent(response);
    }
    else if(len == strlen("VIN") && strncmp(cmd, "VIN", len) == 0){
        return udsRequestVin(response);
    }
    return STATUS_NOT_FOUND;
}
