#ifndef APP_CAN_HAL_H
#define APP_CAN_HAL_H
#include <stdint.h>
#include <stdbool.h> 

#ifdef __cplusplus
extern "C" {
#endif

#define CANRingBufMaxSize         16
#define CAN_CLIENT_TX_ID      0x7E0U
#define CAN_SERVER_RX_ID      0x7E8U
//TODO: Will need to modify for 64 byte CAN FD frames. For now, we will only support 8 byte classic CAN frames.
typedef struct CANMsg{ 
    uint32_t id; 
    uint8_t data[8];
    //Message will not necessarily take all 8 bytes.  
    uint8_t len; 
}CANMsg; 

void CANInit(void);

void CANSend(const uint8_t *data, uint8_t len);

bool CANReceive(CANMsg *msg);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAN_HAL_H */
