#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include <stdint.h> 
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file timer.h 
 * @brief Provides portable wrappers around platform specific code to initialize and get time values from a system clock. 
**/

/**
 * @brief Used to initialize the system clock. Exact implementation is platform specific. 
 * 
 * @retval Status from status.h that represents the success of intializing the system clock. 
**/ 
Status clockInit(void);

/**
 * @brief Used to get the time in MS since the system started from the clock initialized in clockInit(void). Exact implementation is platform specific.
 * 
 * @retval uint32_t that represents the time in MS since the system started. 
**/ 
uint32_t GetTickMS(void); 

/**
 * @brief Used to delay the system by a number of milliseconds. Exact implementation is platform specific.
 * 
 * @param[in] delay A uint32_t that represents the time in MS to delay by. 
**/ 
void delayMS(uint32_t delay); 

#ifdef __cplusplus
}
#endif

#endif /* UTILS_TIMER_H */
