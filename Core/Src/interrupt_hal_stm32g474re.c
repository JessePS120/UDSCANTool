#include "interrupt.h"
#include "cmsis_gcc.h"

uint32_t enterCritical(void){
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

void exitCritical(uint32_t state){
    __set_PRIMASK(state);
}
