#ifndef ERRORS_CAN_ERROR_H
#define ERRORS_CAN_ERROR_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CANERROR{
    HAL_FDCAN_INIT, 
    HAL_FDCAN_CONFIG_FILTER, 
    HAL_FDCAN_CONFIG_GLOBAL_FILTER, 
    HAL_FDCAN_START, 
    HAL_FDCAN_ACTIVATE_NOTIFICATION, 
    HAL_FDCAN_GET_RX_MESSAGE,
    HAL_FDCAN_ADD_MESSAGE_TO_TX_FIFOQ, 
}CANERROR; 

void CANError(CANERROR error); 

#ifdef __cplusplus
}
#endif

#endif /* ERRORS_CAN_ERROR_H */
