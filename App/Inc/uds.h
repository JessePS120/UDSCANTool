#ifndef APP_UDS_H
#define APP_UDS_H

#include <stdint.h>
#include "isotp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDS_POSITIVE_RESPONSE_OFFSET 0x40U
#define UDS_NEGATIVE_RESPONSE_SID    0x7FU

//Standardized Read/WriteDataByIdentifier DID for the Vehicle Identification Number (ISO 14229-1 Annex F).
#define UDS_DID_VIN 0xF190U

typedef enum UDS_SID{
    UDS_SID_READ_DATA_BY_IDENTIFIER = 0x22,
    UDS_SID_TESTER_PRESENT          = 0x3E,
}UDS_SID;

typedef enum UDS_NRC{
    UDS_NRC_GENERAL_REJECT                            = 0x10,
    UDS_NRC_SERVICE_NOT_SUPPORTED                     = 0x11,
    UDS_NRC_SUBFUNCTION_NOT_SUPPORTED                 = 0x12,
    UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT= 0x13,
    UDS_NRC_RESPONSE_TOO_LONG                         = 0x14,
    UDS_NRC_BUSY_REPEAT_REQUEST                       = 0x21,
    UDS_NRC_CONDITIONS_NOT_CORRECT                    = 0x22,
    UDS_NRC_REQUEST_SEQUENCE_ERROR                    = 0x24,
    UDS_NRC_REQUEST_OUT_OF_RANGE                      = 0x31,
    UDS_NRC_RESPONSE_PENDING                          = 0x78,
    UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_SESSION      = 0x7E,
    UDS_NRC_SERVICE_NOT_SUPPORTED_IN_SESSION          = 0x7F,
}UDS_NRC;

const char *UDS_NRCtoString(UDS_NRC nrc);

typedef enum UdsStatus{
    UDS_STATUS_OK,
    UDS_STATUS_NEGATIVE_RESPONSE,
    UDS_STATUS_TIMEOUT,
    UDS_STATUS_TRANSPORT_ERROR,
    UDS_STATUS_INVALID_RESPONSE,
    UDS_STATUS_REQUEST_TOO_LARGE,
}UdsStatus;

const char *UdsStatustoString(UdsStatus status);

typedef struct UdsResult{
    UdsStatus status;
    ISOTPSTATUS isotpStatus;
}UdsResult;

typedef struct UdsResponse{
    uint8_t sid;
    uint8_t nrc;
    uint8_t *data;
    uint16_t len;
}UdsResponse;

UdsResult sendUdsRequest(uint8_t sid, uint8_t *data, uint16_t len, UdsResponse *response);

UdsResult udsTesterPresent(UdsResponse *response);
UdsResult udsRequestVin(UdsResponse *response);

#ifdef __cplusplus
}
#endif

#endif /* APP_UDS_H */
