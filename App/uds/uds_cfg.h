#ifndef APP_UDS_CFG_H
#define APP_UDS_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file uds_cfg.h 
 * @brief Provides a series of macros to configure major UDS functionality. 
 * List of macros: 
 * UDS_P2_CLIENT_MS -> ISO 14229-2 max time(in ms) to wait for the initial response before it is considered a timeout.
 * UDS_P2_STAR_CLIENT_MS -> ISO 14229-2 max time(in ms) to wait after a 0x78 (response pending) NRC before giving up.
 * UDS_REQUEST_SEND_TIMEOUT_MS -> max time(in ms) the UDS module will wait to receive a response before timing out. 
**/

#define UDS_P2_CLIENT_MS 50U

#define UDS_P2_STAR_CLIENT_MS 5000U

#define UDS_REQUEST_SEND_TIMEOUT_MS 1000U

#define MAX_UDS_MSG_SIZE 100 

#ifdef __cplusplus
}
#endif

#endif /* APP_UDS_CFG_H */
