#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

uint32_t GetTickMS(void);

void delayMS(uint32_t delay); 

#ifdef __cplusplus
}
#endif

#endif /* UTILS_TIMER_H */
