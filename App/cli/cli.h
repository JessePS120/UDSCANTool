#ifndef APP_CLI_H
#define APP_CLI_H

#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_CLIENT_TX_ID 0x7E0 
#define CAN_SERVER_RX_ID 0x7E8

//Brings up the peripherals in dependency order and stops at the first failure.
Status clientInit(void); 

//Runs the console loop and never returns.
void clientStart(void); 

#ifdef __cplusplus
}
#endif

#endif /* APP_CLI_H */
