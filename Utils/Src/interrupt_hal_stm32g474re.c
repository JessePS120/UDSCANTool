#include "interrupt.h" 
#include "cmsis_gcc.h"

void enableInterrupts(void){
    __enable_irq();
}

void disableInterrupts(void){
    __disable_irq();
}