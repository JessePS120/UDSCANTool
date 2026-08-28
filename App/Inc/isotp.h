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

void getFrames(IsoTpMsg *msg); 

void sendFrame(uint8_t *data, uint16_t len); 

#ifdef __cplusplus
}
#endif

#endif /* APP_ISOTP_H */
