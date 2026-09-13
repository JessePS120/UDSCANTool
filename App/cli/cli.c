#include "cli.h"
#include "led.h"
#include "uart.h"
#include "uart_buf.h"
#include "can.h"
#include "timer.h"
#include "isotp_port.h"
#include "uds.h"
#include "status.h"
#include "log.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_nucleo.h"
#include <string.h>
#include <stdio.h>

#define CLI_LOG_TAG "CLI"

char UARTBuf[UARTRingBufMaxSize] = {0};

//Reporting the results of a failed command. Not to be confused with the logging system in 
// /Utils which is for more specific error messages. 
static void reportStatus(const char *what, Status status){
    char line[UARTRingBufMaxSize];
    snprintf(line, sizeof(line), "%s failed: %s\r\n", what, StatustoString(status));
    (void)UARTSend(line, 0, true);
}

Status clientInit(void){
    //TODO: move GPIO, system clock and other functions here.
    if(HAL_Init() != HAL_OK){
        return STATUS_HW_FAULT;
    }
    //Nothing below can be reported yet, so these statuses travel back to main() unlogged.
    Status status = clockInit();
    if(status != STATUS_OK){
        return status;
    }
    //LED must be ready next since UART faults are reported by blinking it.
    status = LEDInit();
    if(status != STATUS_OK){
        return status;
    }
    //UART must be ready before CAN since CAN errors are reported over UART by the logger.
    status = UARTInit();
    if(status != STATUS_OK){
        return status;
    }
    status = CANInit();
    if(status != STATUS_OK){
        //CANInit() has already logged the specific failure.
        logDrain();
        return status;
    }
    //init ISO-TP last since it sits on top of CAN. The tester transmits on 0x7E0 and only accepts 0x7E8.
    status = isotpInit(CAN_CLIENT_TX_ID, CAN_SERVER_RX_ID);
    if(status != STATUS_OK){
        logDrain();
        return status;
    }
    LOG_INFO(CLI_LOG_TAG, "peripherals initialised");
    logDrain();
    return STATUS_OK;
}

void clientStart(void){
    uint32_t UARTBytes = 0;
    char stopKey = 0;
    snprintf(UARTBuf, sizeof(UARTBuf), "The current UART buffer Is limited to %u bytes\r\n", (unsigned)sizeof(UARTBuf));
    (void)UARTSend(UARTBuf, 0, true);

    /*
    CANS:[Message Data(cannot be more than 8 bytes!)]
    CANR:(Constantly print recieved CAN messages)
    UDS:[CMD(Must be a valid UDS command as defined in uds/uds.c)]
    */
    while(true){ 
        //Setup for the next cycle of UART reads. 
        logDrain(); 
        UARTFlushBuf(); 
        (void)UARTSend("Enter CMD\r\n", 0, true);
        if(UARTBufPoll(UARTBuf, sizeof(UARTBuf), &UARTBytes) != STATUS_OK){
            LOG_INFO(CLI_LOG_TAG, "issue polling uart buffer");
            continue;
        }
        char *semiChar = strchr(UARTBuf, ':'); 
        if(semiChar == NULL){
            (void)UARTSend("ERROR: No semi-colon detected in message\r\n", 0, true); 
        }
        else{ 
            if(semiChar - UARTBuf == 4 && strncmp(UARTBuf, "CANS", 4) == 0){ 
                Status status = CANSend((const uint8_t *)&UARTBuf[5], CAN_CLIENT_TX_ID, UARTBytes - 5);
                if(status == STATUS_INVALID_ARG){
                    (void)UARTSend("ERROR: cannot send more than 8 bytes over CAN Classic\r\n", 0, true);
                    continue;
                }
                if(status != STATUS_OK){
                    reportStatus("CAN send", status);
                }
            }
            else if(semiChar - UARTBuf == 4 && strncmp(UARTBuf, "CANR", 4) == 0){ 
                UARTFlushBuf();
                (void)UARTSend("Printing received CAN message. Enter any character to stop\r\n", 0, true);
                CANMsg msg = {0};
                while(true){
                    if(UARTReceive(&stopKey) == STATUS_OK){
                        delayMS(50);
                        UARTFlushBuf();
                        break;
                    }
                    Status status = CANReceive(&msg);
                    if(status == STATUS_OK){
                        uint32_t offset = (uint32_t)snprintf(UARTBuf, sizeof(UARTBuf), "Received message from: 0x%03lX Containing:", (unsigned long)msg.id);
                        //Avoid any overflows of the UARTBuf.
                        for(uint8_t i = 0; i < msg.len && offset < sizeof(UARTBuf); i++){
                            offset += (uint32_t)snprintf(UARTBuf + offset, sizeof(UARTBuf) - offset, " %02X", msg.data[i]);
                        }
                        if(offset < sizeof(UARTBuf)){
                            snprintf(UARTBuf + offset, sizeof(UARTBuf) - offset, "\r\n");
                        }
                        (void)UARTSend(UARTBuf, 0, true);
                    }
                    else if(status != STATUS_NO_DATA){
                        reportStatus("CAN receive", status);
                    }
                    logDrain();
                }
            }
            else if(semiChar - UARTBuf == 3 && strncmp(UARTBuf, "UDS", 3) == 0){
                UdsResponse response = {0};
                Status status = sendUDSCmd((const char *)&UARTBuf[4], &response);
                if(status == STATUS_PROTOCOL_ERROR && response.nrc != 0){
                    snprintf(UARTBuf, sizeof(UARTBuf), "UDS rejected: %s\r\n", UDS_NRCtoString((UDS_NRC)response.nrc));
                    (void)UARTSend(UARTBuf, 0, true);
                }
                else if(status != STATUS_OK){
                    reportStatus("UDS command", status);
                }
                else{
                    uint32_t offset = (uint32_t)snprintf(UARTBuf, sizeof(UARTBuf), "UDS response SID: 0x%02X Containing:", response.sid);
                    //Avoid any overflows of the UARTBuf.
                    for(uint16_t i = 0; i < response.len && offset < sizeof(UARTBuf); i++){
                        offset += (uint32_t)snprintf(UARTBuf + offset, sizeof(UARTBuf) - offset, " %02X", response.data[i]);
                    }
                    if(offset < sizeof(UARTBuf)){
                        snprintf(UARTBuf + offset, sizeof(UARTBuf) - offset, "\r\n");
                    }
                    (void)UARTSend(UARTBuf, 0, true);
                }
            } 
            else{ 
                (void)UARTSend("ERROR: Not a valid command before the semi-colon.\r\n", 0, true);
            }
        }
    }
}