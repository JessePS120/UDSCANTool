#ifndef APP_CAN_HAL_H
#define APP_CAN_HAL_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//TODO: Will need to modify for 64 byte CAN FD frames. For now, we will only support 8 byte classic CAN frames.
typedef struct CANMsg{ 
    uint32_t id; 
    uint8_t data[8];
    //Message will not necessarily take all 8 bytes.  
    uint8_t len; 
}CANMsg; 

void can_init(void);

void can_send(uint8_t *data, uint8_t len);

void can_receive(CANMsg *msg); 

#ifdef __cplusplus
}
#endif

#endif /* APP_CAN_HAL_H */
