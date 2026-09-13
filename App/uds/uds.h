#ifndef APP_UDS_H
#define APP_UDS_H

#include "status.h" 
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uds.h 
 * @brief Provides functionality for sending/interpreting UDS messages.
**/

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

/**
 * @brief Function that converts a UDS_NRC to a string that is statically allocated in memory. 
 * @param[in] nrc A UDS_NRC to be converted into a string. 
 * @retval [const char *] that points to the string equivalent of nrc. This string is statically stored in memory. 
 **/ 
const char *UDS_NRCtoString(UDS_NRC nrc);

typedef struct UdsResponse{
    uint8_t sid;
    uint8_t nrc;
    //Points into the module's receive buffer, so read it before issuing the next request.
    uint8_t *data;
    uint16_t len;
}UdsResponse;

/**
 * @brief Function that sends a message using UDS. 
 * @param[in] cmd a const char * that represents a string for the command that should be sent. I.e. "VIN" will send a request for the vehicle VIN. See uds.c for details. 
 * @param[in, out] response UdsResponse * that will be filled with the values sent from the server after the UDS request has been sent. See uds.c for details.  
 * @retval Status variable representing the success of sending the UDS message. See status.h for details. 
 **/ 
Status sendUDSCmd(const char *cmd, UdsResponse *response);

#ifdef __cplusplus
}
#endif

#endif /* APP_UDS_H */
