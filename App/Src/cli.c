#include "cli.h"
#include "led.h"
#include "uart.h"
#include "uart_buf.h"
#include "can.h"
#include "timer.h" 
#include "isotp.h"
#include "uds.h" 
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"
#include <string.h>
#include <stdio.h>


void clientInit(void){
    //LED must be ready first since UART init errors are reported by blinking it.
    //TODO: move GPIO, system clock and other functions here. 
    HAL_Init();
    clockInit(); 
    LEDInit();
    //UART must be ready next since CAN init errors are reported over UART.
    UARTInit();
    CANInit();
    //TODO: add other functions here like the system clock and any GPIO. 
}

char UARTBuf[UARTRingBufMaxSize] = {0};

void clientStart(void){
    uint8_t UARTBytes = 0;
    UARTSend("Waiting for response\r\n", 0, true); 
    UARTBufPoll(UARTBuf, sizeof(UARTBuf)); 
    snprintf((char *)(UARTBuf), sizeof(UARTBuf), "The current UART buffer Is limited to %d bytes\r\n", sizeof(UARTBuf));
    UARTSend(UARTBuf, 0, true); 
    while(1){
        UARTFlushBuf();
        UARTSend("Would you like to send/receive a message(s) over CAN Classic or ISOTP?(CANS/CANR/ISOTPS/ISOTPR/UDS)\r\n", 0, true);
        UARTBufPoll(UARTBuf, sizeof(UARTBuf));
        if(strcmp(UARTBuf, "CANS") == 0){
            UARTFlushBuf(); 
            while(1){
                UARTSend("Enter the byte(s) you would like to send!\r\n", 0, true);
                UARTBytes = UARTBufPoll(UARTBuf, sizeof(UARTBuf)); 
                if(UARTBytes > 8){
                    UARTSend("Error, cannot send more than 8 bytes over CAN Classic\r\n", 0, true);
                }
                else{
                    CANSend((uint8_t *)UARTBuf, UARTBytes);
                    delayMS(500);
                    break;
                }
            }
        }
        else if(strcmp(UARTBuf, "ISOTPS") == 0){
            UARTFlushBuf(); 
            UARTSend("Enter the byte(s) you would like to send!\r\n", 0, true);
            UARTBytes = UARTBufPoll(UARTBuf, sizeof(UARTBuf)); 
            ISOTPSTATUS status = sendFrame((uint8_t *)UARTBuf, UARTBytes);
            if(status != ISOTP_STATUS_OK){ 
                snprintf((char *)UARTBuf, sizeof(UARTBuf), "%s\r\n", ISOTPSTATUStoString(status));
                UARTSend(UARTBuf, 0, true); 
            }
            delayMS(500);
        }
        else if(strcmp(UARTBuf, "CANR") == 0){
            UARTFlushBuf(); 
            UARTSend("Printing received CAN message. Enter any character to stop\r\n", 0, true);
            CANMsg msg = {0};
            while(1){
                if(UARTReceive((char *)&UARTBytes)){
                    delayMS(50); 
                    UARTFlushBuf(); 
                    break;
                }
                if(CANReceive(&msg)){
                    uint32_t offset = (uint32_t)snprintf((char *)UARTBuf, sizeof(UARTBuf), "Received message from: 0x%03lX Containing:", (unsigned long)msg.id);
                    //Avoid any overflows of the UARTBuf.
                    for(uint8_t i = 0; i < msg.len && offset < sizeof(UARTBuf); i++){
                        offset += (uint32_t)snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, " %02X", msg.data[i]);
                    }
                    if(offset < sizeof(UARTBuf)){
                        snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, "\r\n");
                    }
                    UARTSend(UARTBuf, 0, true);
                }
            }
        }
        else if(strcmp(UARTBuf, "ISOTPR") == 0){
            UARTFlushBuf(); 
            UARTSend("Printing received ISOTP message. Enter any character to stop\r\n", 0, true);
            IsoTpMsg msg = {0};
            while(1){
                if(UARTReceive((char *)&UARTBytes)){
                    delayMS(50); 
                    UARTFlushBuf(); 
                    break;
                }
                ISOTPSTATUS status = getFrames(&msg, 250);
                if(status != ISOTP_STATUS_OK){ 
                    snprintf((char *)UARTBuf, sizeof(UARTBuf), "%s\r\n", ISOTPSTATUStoString(status)); 
                    UARTSend(UARTBuf, 0, true); 
                }
                if(msg.len > 0){
                    uint32_t offset = (uint32_t)snprintf((char *)UARTBuf, sizeof(UARTBuf), "Received message from: 0x%03lX Containing:", (unsigned long)msg.id);
                    //Avoid any overflows of the UARTBuf.
                    for(uint16_t i = 0; i < msg.len && offset < sizeof(UARTBuf); i++){
                        offset += (uint32_t)snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, " %02X", msg.data[i]);
                    }
                    if(offset < sizeof(UARTBuf)){
                        snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, "\r\n");
                    }
                    UARTSend(UARTBuf, 0, true);
                }
            }
        }
        else if(strcmp(UARTBuf, "UDS") == 0){
            UARTFlushBuf();
            UARTSend("Enter the message you would like to send!\r\n", 0, true);
            UARTBytes = UARTBufPoll(UARTBuf, sizeof(UARTBuf));
            UdsResponse response = {0};
            UdsResult result = sendUDSCmd(UARTBuf, &response);
            if(result.UDSStatus != UDS_STATUS_OK || result.isotpStatus != ISOTP_STATUS_OK){
                snprintf((char *)(UARTBuf), sizeof(UARTBuf), "%s %s\r\n", UdsStatustoString(result.UDSStatus), ISOTPSTATUStoString(result.isotpStatus));
                UARTSend(UARTBuf, 0, true);
            }
            else{
                uint32_t offset = (uint32_t)snprintf((char *)UARTBuf, sizeof(UARTBuf), "UDS response SID: 0x%02X Containing:", response.sid);
                //Avoid any overflows of the UARTBuf.
                for(uint16_t i = 0; i < response.len && offset < sizeof(UARTBuf); i++){
                    offset += (uint32_t)snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, " %02X", response.data[i]);
                }
                if(offset < sizeof(UARTBuf)){
                    snprintf((char *)UARTBuf + offset, sizeof(UARTBuf) - offset, "\r\n");
                }
                UARTSend(UARTBuf, 0, true);
            }
        }
        else{
            UARTSend("Not a valid command, please try again!\r\n", 0, true);
        }
    }
}
