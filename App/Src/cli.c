#include "cli.h"
#include "led.h"
#include "uart.h"
#include "can.h"


void clientInit(void){
    //LED must be ready first since UART init errors are reported by blinking it.
    //TODO: move GPIO, system clock and other functions here. 
    LEDInit();
    //UART must be ready next since CAN init errors are reported over UART.
    UARTInit();
    CANInit();
}

void clientStart(void); 