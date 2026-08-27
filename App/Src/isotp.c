#include "isotp.h"
#include "can.h" 

CANMsg[ISOTP_MAX_MESSAGE_SIZE] isotpBuf = {0}; 

void sendFlowCtrlFrame(uint8_t flowStatus, uint8_t blockSize, uint8_t stMin){ 
    CANMsg flowCtrlFrame; 
    flowCtrlFrame.id = CAN_CLIENT_TX_ID; 
    flowCtrlFrame.data[0] = 0x30 | (flowStatus & 0x0F); 
    flowCtrlFrame.data[1] = blockSize; 
    flowCtrlFrame.data[2] = stMin; 
    flowCtrlFrame.len = 3; 
    CANSend(flowCtrlFrame.data, flowCtrlFrame.len); 
}

void receiveConsecutiveFrame

void getFrames(CANMsg *msg, uint8_t *frameCount){ 
    CANMsg frame; 
    *frameCount = 0; 
    CANReceive(&frame);
    msg = NULL 
    if(&frame == NULL){ 
        return; 
    }
    *frameCount++; 
    if(frame.data[0] & 0xF0 == 0x00){ 
        //Single frame message. 
        isotpBuf[0] = frame; 
        msg = &isotpBuf[0];
    }
    else if(frame.data[0] & 0xF0 == 0x10){ 
        //First frame message. 
    }
    else{ 
        //Invalid frame type. Need some error handling here. 
    }
}

