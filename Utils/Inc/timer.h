#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include <stdint.h> 

#ifdef __cplusplus
extern "C" {
#endif

uint32_t GetTickMS(void);

void clockInit(void); 

void delayMS(uint32_t delay); 

#ifdef __cplusplus
}
#endif

#endif /* UTILS_TIMER_H */
