#include "isotp.h"
#include "isotp_cfg.h"
#include "can.h"
#include "timer.h"
#include <stdint.h>
#include <string.h>  
#include <stdbool.h>

uint8_t isotpBuf[4095] = {0};

//Indexed by designated initializer so the mapping stays correct even if ISOTPSTATUS
//is reordered or given gaps later; any enumerator left without an initializer here
//defaults to NULL, which the bounds/NULL check below catches.
static const char *const isotpStatusStrings[] = {
    [ISOTP_N_CR_TIMEOUT]                   = "ISOTP_N_CR_TIMEOUT",
    [ISOTP_INVALID_CONSECUTIVE_FRAME]      = "ISOTP_INVALID_CONSECUTIVE_FRAME",
    [ISOTP_SEQUENCE_NUMBER_MISMATCH]       = "ISOTP_SEQUENCE_NUMBER_MISMATCH",
    [ISOTP_NO_FRAME_RECEIVED]              = "ISOTP_NO_FRAME_RECEIVED",
    [ISOTP_INVALID_FIRST_FRAME_LENGTH]     = "ISOTP_INVALID_FIRST_FRAME_LENGTH",
    [ISOTP_INVALID_FRAME_TYPE]             = "ISOTP_INVALID_FRAME_TYPE",
    [ISOTP_N_BS_TIMEOUT]                   = "ISOTP_N_BS_TIMEOUT",
    [ISOTP_INVALID_FLOW_CONTROL_FRAME]     = "ISOTP_INVALID_FLOW_CONTROL_FRAME",
    [ISOTP_FLOW_CONTROL_FRAME_MAX_EXCEEDED]= "ISOTP_FLOW_CONTROL_FRAME_MAX_EXCEEDED",
    [ISOTP_FLOW_CONTROL_OVERFLOW_ABORT]    = "ISOTP_FLOW_CONTROL_OVERFLOW_ABORT",
    [ISOTP_INVALID_FLOW_CONTROL_STATUS]    = "ISOTP_INVALID_FLOW_CONTROL_STATUS",
    [ISOTP_STATUS_OK]                      = "ISOTP_STATUS_OK",
};

const char *ISOTPSTATUStoString(ISOTPSTATUS status){
    uint8_t index = (uint8_t)status;
    if(index >= (sizeof(isotpStatusStrings) / sizeof(isotpStatusStrings[0])) || isotpStatusStrings[index] == NULL){
        return "ISOTP_STATUS_UNKNOWN";
    }
    return isotpStatusStrings[index];
}

static void sendFlowCtrlFrame(uint8_t flowStatus, uint8_t blockSize, uint8_t stMin){ 
    CANMsg flowCtrlFrame; 
    flowCtrlFrame.id = CAN_CLIENT_TX_ID; 
    flowCtrlFrame.data[0] = 0x30 | (flowStatus & 0x0F); 
    flowCtrlFrame.data[1] = blockSize; 
    flowCtrlFrame.data[2] = stMin; 
    flowCtrlFrame.len = 3; 
    CANSend(flowCtrlFrame.data, flowCtrlFrame.len); 
}

static ISOTPSTATUS receiveConsecutiveFrame(IsoTpMsg *msg, uint16_t remainingBytes){ 
    //Index for consecutive frames should start at 1. 
    uint8_t seqNum = 1; 
    while(remainingBytes > 0){ 
        CANMsg frame;
        uint32_t deadline = GetTickMS() + ISOTP_N_CR_MS;
        while(!CANReceive(&frame)){
            if(GetTickMS() >= deadline){
                //N_Cr timeout sender didn't deliver the next CF in time.
                return ISOTP_N_CR_TIMEOUT; 
            }
        }
        if((frame.data[0] & 0xF0) != 0x20){
            //Not a consecutive frame.
            return ISOTP_INVALID_CONSECUTIVE_FRAME;
        }
        if(((frame.data[0] & 0x0F) != seqNum)){
            //Sequence number does not match expected value.
            return ISOTP_SEQUENCE_NUMBER_MISMATCH;
        }
        uint16_t bytesToCopy = (remainingBytes < 7) ? remainingBytes : 7;
        memcpy(&isotpBuf[msg->len-remainingBytes], &frame.data[1], bytesToCopy);
        remainingBytes -= bytesToCopy;
        //This is a four bit rolling counter.
        seqNum = (seqNum + 1) & 0x0F;
    }
    return ISOTP_STATUS_OK;
}

ISOTPSTATUS getFrames(IsoTpMsg *msg, uint32_t timeoutMs){
    msg->len = 0;
    msg->data = NULL;
    CANMsg frame;
    uint32_t deadline = GetTickMS() + timeoutMs;
    while(!CANReceive(&frame)){
        if(GetTickMS() >= deadline){
            //No frame arrived within the timeout window, keep msg->data NULL and return.
            return ISOTP_NO_FRAME_RECEIVED;  
        }
    }
    msg->data = &isotpBuf[0];
    msg->id = frame.id;
    //Single frame message.
    if((frame.data[0] & 0xF0) == 0x00){  
        //Low nibble of byte 0 is the length of the message. 
        msg->len = frame.data[0] & 0x0F;
        memcpy(isotpBuf, &frame.data[1], msg->len); 
    }
    //First frame message.
    else if((frame.data[0] & 0xF0) == 0x10){ 
        msg->len = ((frame.data[0] & 0x0F) << 8) | frame.data[1]; 
        if(msg->len <= 6){
            //Not a valid first frame message as it should be a single frame.
            return ISOTP_INVALID_FIRST_FRAME_LENGTH; 
        }
        else{
            memcpy(isotpBuf, &frame.data[2], 6);
            sendFlowCtrlFrame(0x00, BLOCK_SIZE, STMIN);
            ISOTPSTATUS status = receiveConsecutiveFrame(msg, msg->len - 6);
            if(status != ISOTP_STATUS_OK){
                return status;
            }
        }
    }
    else{
        //Invalid frame type.
        return ISOTP_INVALID_FRAME_TYPE;
    }
    return ISOTP_STATUS_OK;
}

ISOTPSTATUS sendFrame(const uint8_t *data, uint16_t len){ 
    uint8_t frame[8] = {0}; 
    if(len <= 7){ 
        //Send single frame message. 
        frame[0] = len & 0x0F;
        memcpy(&frame[1], data, len);
        CANSend(frame, len + 1);
        return ISOTP_STATUS_OK;
    }
    else{ 
        //Send first frame. 
        frame[0] = 0x10 | ((len >> 8) & 0x0F);
        frame[1] = len & 0xFF;
        memcpy(&frame[2], data, 6); 
        CANSend(frame, 8);
        uint16_t remainingBytes = len - 6;
        uint32_t deadline; 
        CANMsg flowCtrlFrame;
        uint8_t flowCtrlFrameCount = 0; 
        uint8_t fcFlag = 0;
        uint8_t blockSize = 0; 
        uint8_t stMin = 0; 
        uint8_t seqNum = 1;  
        uint8_t framesSentInBlock = 0; 
        uint8_t bytesToSend = 0; 

        while(1){
            deadline = GetTickMS() + ISOTP_N_BS_MS;
            while(!CANReceive(&flowCtrlFrame)){
                if(GetTickMS() >= deadline){
                    //N_BS timeout receiver didn't deliver the flow control frame in time.
                    return ISOTP_N_BS_TIMEOUT; 
                }
            }
            if((flowCtrlFrame.data[0] & 0xF0) != 0x30){
                //Not a flow control frame.
                return ISOTP_INVALID_FLOW_CONTROL_FRAME; 
            }
            if(flowCtrlFrameCount >= ISOTP_FLOW_CONTROL_FRAME_MAX){
                //Exceeded max number of flow control frames.
                return ISOTP_FLOW_CONTROL_FRAME_MAX_EXCEEDED; 
            }
            flowCtrlFrameCount++;
            fcFlag = flowCtrlFrame.data[0] & 0x0F;
            if (fcFlag == 0x02){
                //Overflow/abort flow control frame.
                return ISOTP_FLOW_CONTROL_OVERFLOW_ABORT; 
            }
            else if(fcFlag == 0x01){
                continue;
            }
            else if(fcFlag != 0x0){
                //Handle all other possible FC values.
                return ISOTP_INVALID_FLOW_CONTROL_STATUS; 
            }
            //We will only get here if a flow control frame allowing us to continue is recieved. 
            //Flow control frame received, send consecutive frames. 
            flowCtrlFrameCount = 0; 
            blockSize = flowCtrlFrame.data[1]; 
            stMin = flowCtrlFrame.data[2];  
            framesSentInBlock = 0; 
            while(remainingBytes > 0 && ((framesSentInBlock < blockSize) || (blockSize == 0))){ 
                bytesToSend = (remainingBytes < 7) ? remainingBytes : 7; 
                frame[0] = 0x20 | (seqNum & 0x0F); 
                memcpy(&frame[1], &data[len - remainingBytes], bytesToSend); 
                CANSend(frame, bytesToSend + 1); 
                remainingBytes -= bytesToSend; 
                framesSentInBlock++; 
                seqNum = (seqNum + 1) & 0x0F;
                //Delay for STmin. Adding some buffer here for any timing inconsistencies. 
                delayMS(stMin+10); 
            }
            if(remainingBytes > 0){
                continue;
            }
            break;
        }
        return ISOTP_STATUS_OK;
    }
}

