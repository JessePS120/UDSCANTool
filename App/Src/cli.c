#include "cli.h"
#include "led.h"
#include "uart.h"
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

//TODO: This code needs to be refactored so that the UART module handles its own buffer and exposes a set of commands 
//to append to this buffer. 
void clientStart(void){
    char UARTBuf[UARTRingBufMaxSize] = {0};
    uint8_t UARTBytes = 0;
    UARTSend("Waiting for response\r\n", 0, true); 
    do{
        UARTBytes = UARTReceiveBuffer(UARTBuf);
        delayMS(250); 
    } while(UARTBytes == 0U);
    
    snprintf((char *)(UARTBuf), sizeof(UARTBuf), "The current UART buffer Is limited to %d bytes\r\n", sizeof(UARTBuf));
    UARTSend(UARTBuf, 0, true); 
    while(1){
        UARTSend("Would you like to send/receive a message(s) over CAN Classic or ISOTP?(CANS/CANR/ISOTPS/ISOTPR/UDS)\r\n", 0, true);
        do{
            delayMS(250);
            UARTBytes = UARTReceiveBuffer(UARTBuf); 
        } while(UARTBytes == 0);
        if(UARTBytes >= 4 && (strncmp(UARTBuf, "CANS", 4) == 0)){
            while(1){
                UARTSend("Enter the byte(s) you would like to send!\r\n", 0, true);
                do{
                    UARTBytes = UARTReceiveBuffer(UARTBuf);
                    delayMS(250); 
                } while(UARTBytes == 0);
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
        else if(UARTBytes >= 6 && (strncmp(UARTBuf, "ISOTPS", 6) == 0)){
            UARTSend("Enter the byte(s) you would like to send!\r\n", 0, true);
            do{
                UARTBytes = UARTReceiveBuffer(UARTBuf);
                delayMS(250); 
            } while(UARTBytes == 0);
            ISOTPSTATUS status = sendFrame((uint8_t *)UARTBuf, UARTBytes);
            if(status != ISOTP_STATUS_OK){ 
                UARTSend(ISOTPSTATUStoString(status), 0, true); 
            }
            delayMS(500);
        }
        else if(UARTBytes >= 4 && (strncmp(UARTBuf, "CANR", 4) == 0)){
            UARTSend("Printing received CAN message. Enter any character to stop\r\n", 0, true);
            CANMsg msg = {0};
            while(1){
                UARTBytes = UARTReceiveBuffer(UARTBuf);
                if(UARTBytes > 0){
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
        else if(UARTBytes >= 6 && (strncmp(UARTBuf, "ISOTPR", 6) == 0)){
            UARTSend("Printing received ISOTP message. Enter any character to stop\r\n", 0, true);
            IsoTpMsg msg = {0};
            while(1){
                UARTBytes = UARTReceiveBuffer(UARTBuf);
                if(UARTBytes > 0){
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
        else if(UARTBytes >= 3 && (strncmp(UARTBuf, "UDS", 3) == 0)){ 
            UARTSend("Enter the message you would like to send!\r\n", 0, true);
            do{
                UARTBytes = UARTReceiveBuffer(UARTBuf);
                delayMS(250);
            } while(UARTBytes == 0);
            //UARTReceiveBuffer does not null-terminate; sendUDSCmd expects a C string.
            UARTBuf[(UARTBytes < sizeof(UARTBuf)) ? UARTBytes : sizeof(UARTBuf) - 1] = '\0';
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
