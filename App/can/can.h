#ifndef APP_CAN_HAL_H
#define APP_CAN_HAL_H
#include <stdint.h>
#include <stdbool.h> 
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file can.h 
 * @brief Provides portable wrappers around platform specific code to initialize and utilize CAN. 
 * List of macros:
 * CANRingBufMaxSize -> Size of the ring buffer(in number of CANMsg structs) used to store messages received from CAN.  
**/

#define CANRingBufMaxSize  16
typedef struct CANMsg{ 
    uint32_t id; 
    uint8_t data[8];
    //Message will not necessarily take all 8 bytes.  
    uint8_t len; 
}CANMsg; 

/**
 * @brief Function that initializes the hardware needed for CAN. 
 * @retval Status enum that represents the success of initializing the CAN hardware. See status.h for details. 
 **/
Status CANInit(void);

/**
 * @brief Function for sending data over CAN. 
 * @param[in] data a const uint8_t * pointing to the bytes to be sent over CAN.  
 * @param[in] id a uint32_t that represents the id used to send the data over CAN. 
 * @param[in] len a uint8_t that represents the number of bytes to send over CAN. IMPORTANT: must not exceeed 8.
 * @retval Status enum that represents the success of sending over CAN. See status.h for details. 
 **/
Status CANSend(const uint8_t *data, uint32_t id, uint8_t len);

/**
 * @brief Function for receving data from the CAN ring buffer. 
 * @param[in, out] msg a CANMsg * which will be filled if the receive operation is successful. 
 * @retval Status enum that represents the success of receiving a message from the CAN ring buffer. See status.h for details. 
 **/
Status CANReceive(CANMsg *msg);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAN_HAL_H */
