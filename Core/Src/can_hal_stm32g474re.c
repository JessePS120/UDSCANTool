#include "can.h"
#include "status.h"
#include "log.h"
#include "stm32g4xx_hal.h"
#include "interrupt.h"
#include <stdint.h>
#include <stdbool.h> 
#include <stddef.h>
#include <string.h> 

#define CAN_LOG_TAG "CAN"

typedef struct canFrameWrapper{ 
    FDCAN_RxHeaderTypeDef header; 
    uint8_t data[8];
}canFrameWrapper; 

static volatile canFrameWrapper rxRingBuf[CANRingBufMaxSize]; 
static volatile uint8_t rxRingBufHead = 0;
static volatile uint8_t rxRingBufTail = 0;
static volatile uint8_t rxRingBufSize = 0; 
static volatile bool dropFrames = true; 

static FDCAN_HandleTypeDef hfdcan = {0}; 

Status CANInit(void)
{
    /* TODO: allow user to send commands from console to configure CAN peripheral */
    hfdcan.Instance = FDCAN1;
    hfdcan.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission = DISABLE;
    hfdcan.Init.TransmitPause = DISABLE;
    hfdcan.Init.ProtocolException = ENABLE;
    /* 500 kbit/s: FDCAN is clocked from PCLK1 at 170 MHz, so a 34 tq bit
     * (1 sync + 29 + 4) with a prescaler of 10 gives 170e6 / 10 / 34 exactly.
     * The sample point lands at 88.2%. */
    hfdcan.Init.NominalPrescaler = 10;
    hfdcan.Init.NominalSyncJumpWidth = 4;
    hfdcan.Init.NominalTimeSeg1 = 29;
    hfdcan.Init.NominalTimeSeg2 = 4;
    hfdcan.Init.DataPrescaler = 1;
    hfdcan.Init.DataSyncJumpWidth = 1;
    hfdcan.Init.DataTimeSeg1 = 13;
    hfdcan.Init.DataTimeSeg2 = 3;
    //No hardware acceptance filters are configured; see the global filter below.
    hfdcan.Init.StdFiltersNbr = 0;
    hfdcan.Init.ExtFiltersNbr = 0;
    hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&hfdcan) != HAL_OK){
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_Init failed");
        return STATUS_HW_FAULT;
    }

    /*FDCAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = CAN_SERVER_RX_ID;
    sFilterConfig.FilterID2 = 0x7FFU; 
    if (HAL_FDCAN_ConfigFilter(&hfdcan, &sFilterConfig) != HAL_OK){
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_ConfigFilter failed");
        return STATUS_HW_FAULT;
    }*/

    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK){
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_ConfigGlobalFilter failed");
        return STATUS_HW_FAULT;
    }

    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK){
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_Start failed");
        return STATUS_HW_FAULT;
    }

    if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK){
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_ActivateNotification failed");
        return STATUS_HW_FAULT;
    }

    //May need to change this priority later. 
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    return STATUS_OK;
}

void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs){ 
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0U){ 
        //We will overwrite old frames if the nucleboard cannot keep up. 
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, (FDCAN_RxHeaderTypeDef*)&rxRingBuf[rxRingBufTail].header, (uint8_t *)rxRingBuf[rxRingBufTail].data) != HAL_OK){
            //Nothing was written into the slot, so do not publish it.
            LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_GetRxMessage failed");
            return;
        }
        rxRingBufTail = (rxRingBufTail + 1) % CANRingBufMaxSize;
        //Keep tracking of if we have sent the drop frames message or not to avoid overwhelming the logger. 
        if (rxRingBufSize >= CANRingBufMaxSize && !dropFrames){
            LOG_WARN(CAN_LOG_TAG, "rx ring buffer full, beginning to drop frames");
            dropFrames = true; 
            return;
        }
        if(dropFrames){ 
            dropFrames = false;
            LOG_INFO(CAN_LOG_TAG, "no longer dropping frames");
        }
        rxRingBufSize++;
    }
}

Status CANSend(const uint8_t *data, uint32_t id, uint8_t len){ 
    if (data == NULL){
        return STATUS_INVALID_ARG;
    }
    //Must enforce 8 byte limit. Not sure what happens otherwise. 
    if (len > 8U){
        LOG_ERROR(CAN_LOG_TAG, "%u byte payload exceeds the 8 byte classic CAN limit", (unsigned)len);
        return STATUS_INVALID_ARG;
    }
    FDCAN_TxHeaderTypeDef txHeader;
    txHeader.Identifier = id;
    //TODO: should modify this later to support 29 bit frames. 
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = len; 
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_CLASSIC_CAN;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &txHeader, data) != HAL_OK){
        //A full TX FIFO is the common case here and the caller can retry, so it is not a hard fault.
        if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan) == 0U){
            return STATUS_BUSY;
        }
        LOG_ERROR(CAN_LOG_TAG, "HAL_FDCAN_AddMessageToTxFifoQ failed");
        return STATUS_HW_FAULT;
    }
    return STATUS_OK;
}

Status CANReceive(CANMsg *msg){
    if (msg == NULL){
        return STATUS_INVALID_ARG;
    }
    if (rxRingBufSize == 0U){ 
        //No messages in the ring buffer. 
        return STATUS_NO_DATA; 
    }
    uint32_t state = enterCritical();
    uint8_t len = (uint8_t)rxRingBuf[rxRingBufHead].header.DataLength;
    if (len > sizeof(msg->data)){
        len = (uint8_t)sizeof(msg->data);
    }
    //Safe because we have disabled interrutps.
    memcpy(msg->data, (const void *)rxRingBuf[rxRingBufHead].data, len);
    msg->id = rxRingBuf[rxRingBufHead].header.Identifier;
    msg->len = len;
    rxRingBufHead = (rxRingBufHead + 1) % CANRingBufMaxSize; 
    rxRingBufSize--; 
    exitCritical(state);
    return STATUS_OK; 
}
