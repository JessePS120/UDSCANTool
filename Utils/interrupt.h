#ifndef UTILS_INTERRUPT_H
#define UTILS_INTERRUPT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file interrupt.h 
 * @brief Provides portable wrappers around platform specific code to enable/disable interrupts. 
 */

/**
 * @brief Used to disable all interrupts after entering a critical section. 
 *
 * @retval uint32_t value that can be used to re-enabled interrupts. Some devices may not require this so the 
 * exact value is platform dependent. For the nucleo this is the value returned by  __get_PRIMASK(). 
 *
**/ 
uint32_t enterCritical(void);

/**
 * @brief Used to enable all interrupts after exiting a critical section. 
 *
 * @param[in] state A 32-bit value that can be used to re-enable interrupts. Some devices may not require 
 * this so the use is platform dependent. For the nucleo this is the value required by __set_PRIMASK() to 
 * re-enable interrupts. 
 *
**/ 
void exitCritical(uint32_t state);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_INTERRUPT_H */
