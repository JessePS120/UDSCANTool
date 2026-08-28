#include "can.h"
#include "can_error.h"
#include "cmsis_gcc.h"
#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h> 
#include <string.h> 

#define rxRingBufMaxSize 16

typedef struct canFrameWrapper{ 
    FDCAN_RxHeaderTypeDef header; 
    uint8_t data[8];
}canFrameWrapper; 

static volatile canFrameWrapper rxRingBuf[rxRingBufMaxSize]; 
static volatile uint8_t rxRingBufHead = 0;
static volatile uint8_t rxRingBufTail = 0;
static volatile uint8_t rxRingBufSize = 0; 

static FDCAN_HandleTypeDef hfdcan = {0}; 

void CANInit(void)
{
    FDCAN_FilterTypeDef sFilterConfig = {0};

    /* TODO: allow user to send commands from console to configure CAN peripheral */
    hfdcan.Instance = FDCAN1;
    hfdcan.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission = ENABLE;
    hfdcan.Init.TransmitPause = DISABLE;
    hfdcan.Init.ProtocolException = ENABLE;
    hfdcan.Init.NominalPrescaler = 16;
    hfdcan.Init.NominalSyncJumpWidth = 1;
    hfdcan.Init.NominalTimeSeg1 = 1;
    hfdcan.Init.NominalTimeSeg2 = 1;
    hfdcan.Init.DataPrescaler = 1;
    hfdcan.Init.DataSyncJumpWidth = 1;
    hfdcan.Init.DataTimeSeg1 = 13;
    hfdcan.Init.DataTimeSeg2 = 3;
    hfdcan.Init.StdFiltersNbr = 1;
    hfdcan.Init.ExtFiltersNbr = 0;
    hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&hfdcan) != HAL_OK){
        CANError(HAL_FDCAN_INIT); 
    }

    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = CAN_SERVER_RX_ID;
    sFilterConfig.FilterID2 = 0x7FFU; 
    if (HAL_FDCAN_ConfigFilter(&hfdcan, &sFilterConfig) != HAL_OK){
        CANError(HAL_FDCAN_CONFIG_FILTER); 
    }

    /* Reject anything not matched by the filter above; UDS doesn't use
     * remote frames. */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan, FDCAN_REJECT, FDCAN_REJECT,FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK){
        CANError(HAL_FDCAN_CONFIG_GLOBAL_FILTER);
    }

    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK){
        CANError(HAL_FDCAN_START);
    }

    if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK){
        CANError(HAL_FDCAN_ACTIVATE_NOTIFICATION);
    }

    //May need to change this priority later. 
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
}

void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs){ 
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0U){ 
        //Avoid copying messages into generic CANmsg struct until the user calls can_receive(). 
        //Using local not global scope for hfdcan. 
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxRingBuf[rxRingBufTail].header, rxRingBuf[rxRingBufTail].data) != HAL_OK){
           CANError(HAL_FDCAN_GET_RX_MESSAGE);
        }
        rxRingBufTail = (rxRingBufTail + 1) % rxRingBufMaxSize;
        //Ensure no overflow on rxRingBufSize. 
        rxRingBufSize = (rxRingBufSize + 1 < rxRingBufMaxSize) ? rxRingBufSize + 1 : rxRingBufSize; 
    }
}

void CANSend(uint8_t *data, uint8_t len){ 
    FDCAN_TxHeaderTypeDef txHeader;
    txHeader.Identifier = CAN_CLIENT_TX_ID;
    //TODO: should modify this later to support 29 bit frames. 
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = len; 
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_CLASSIC_CAN;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &txHeader, data) != HAL_OK){
        CANError(HAL_FDCAN_ADD_MESSAGE_TO_TX_FIFOQ);
    }
}

bool CANReceive(CANMsg *msg){
    if (rxRingBufSize == 0U){ 
        //No messages in the ring buffer. 
        return false; 
    }
    __disable_irq();
    memcpy(msg->data, rxRingBuf[rxRingBufHead].data, rxRingBuf[rxRingBufHead].header.DataLength);
    msg->id = rxRingBuf[rxRingBufHead].header.Identifier;
    msg->len = rxRingBuf[rxRingBufHead].header.DataLength;
    rxRingBufHead = (rxRingBufHead + 1) % rxRingBufMaxSize; 
    rxRingBufSize--; 
    __enable_irq();
    return true; 
}
