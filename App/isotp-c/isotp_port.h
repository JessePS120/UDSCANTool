#ifndef APP_ISOTPC_H
#define APP_ISOTPC_H

#include <stdint.h>
#include "status.h" 

#ifdef __cplusplus
extern "C" {
#endif

//Sizes used for isotp-c's buffers. 
#define ISOTP_SEND_BUF_SIZE 100
#define ISOTP_REC_BUF_SIZE 100 

Status isotpInit(uint32_t txID, uint32_t rxID);  

Status isotpPump(void);

Status isotpSend(const uint8_t *msg, uint32_t len, uint32_t timeoutMs); 

Status isotpRec(uint8_t *buf, uint32_t bufLen, uint32_t *bytesCopied, uint32_t timeoutMs);  

#ifdef __cplusplus
}
#endif

#endif /* APP_ISOTPC_H */
