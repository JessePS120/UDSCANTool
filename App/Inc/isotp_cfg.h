#ifndef APP_ISOTP_CFG_H
#define APP_ISOTP_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

//ISO 15765-2 N_Cr: max time to wait for the next consecutive frame before aborting reception.
#define ISOTP_N_CR_MS 1000U
#define ISOTP_N_BS_MS 1000U 
#define BLOCK_SIZE 0U
#define STMIN 0U

//Number of flow control frames client is willing to receive before aborting. 
#define ISOTP_FLOW_CONTROL_FRAME_MAX 15 

#ifdef __cplusplus
}
#endif

#endif /* APP_ISOTP_CFG_H */
