#ifndef APP_UDS_CFG_H
#define APP_UDS_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

//ISO 14229-2 P2Client: max time to wait for the initial response before it is considered a timeout.
#define UDS_P2_CLIENT_MS 50U
//ISO 14229-2 P2*Client: max time to wait after a 0x78 (response pending) NRC before giving up.
#define UDS_P2_STAR_CLIENT_MS 5000U

#ifdef __cplusplus
}
#endif

#endif /* APP_UDS_CFG_H */
