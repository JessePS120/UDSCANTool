#include "can_error.h"
#include "interrupt.h"
#include "uart.h"

static const char *const errorMessages[] = {
    [HAL_FDCAN_INIT]                    = "CAN error: HAL_FDCAN_Init failed\r\n",
    [HAL_FDCAN_CONFIG_FILTER]           = "CAN error: HAL_FDCAN_ConfigFilter failed\r\n",
    [HAL_FDCAN_CONFIG_GLOBAL_FILTER]    = "CAN error: HAL_FDCAN_ConfigGlobalFilter failed\r\n",
    [HAL_FDCAN_START]                   = "CAN error: HAL_FDCAN_Start failed\r\n",
    [HAL_FDCAN_ACTIVATE_NOTIFICATION]   = "CAN error: HAL_FDCAN_ActivateNotification failed\r\n",
    [HAL_FDCAN_GET_RX_MESSAGE]          = "CAN error: HAL_FDCAN_GetRxMessage failed\r\n",
    [HAL_FDCAN_ADD_MESSAGE_TO_TX_FIFOQ] = "CAN error: HAL_FDCAN_AddMessageToTxFifoQ failed\r\n",
};

void CANError(CANERROR error){
    disableInterrupts();

    UARTSend((uint8_t *)errorMessages[error], 0, true);

    while(1)
        ;

}