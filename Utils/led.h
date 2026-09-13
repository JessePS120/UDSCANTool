#ifndef UTILS_LED_H
#define UTILS_LED_H

#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file led.h 
 * @brief Provides portable wrappers around platform specific code to initialize and toggle an LED. 
**/

/**
 * @brief Used to initialize any LED. Exact implementation is platform specific. 
 * 
 *
**/ 
Status LEDInit(void);

/**
 * @brief Used to toggle the LED initialized in LEDInit. Exact implementation is platform specific. 
 * 
 *
**/ 
void toggleLED(void); 

#ifdef __cplusplus
}
#endif

#endif /* UTILS_LED_H */
