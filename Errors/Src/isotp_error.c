#include "isotp_error.h"
#include "uart.h"

static const char *const errorMessages[] = {
    [ISOTP_N_CR_TIMEOUT]                    = "ISO-TP error: N_Cr timeout waiting for consecutive frame\r\n",
    [ISOTP_INVALID_CONSECUTIVE_FRAME]       = "ISO-TP error: received frame is not a consecutive frame\r\n",
    [ISOTP_SEQUENCE_NUMBER_MISMATCH]        = "ISO-TP error: consecutive frame sequence number mismatch\r\n",
    [ISOTP_INVALID_FIRST_FRAME_LENGTH]      = "ISO-TP error: first frame length too small for a multi-frame message\r\n",
    [ISOTP_INVALID_FRAME_TYPE]              = "ISO-TP error: received frame has an invalid frame type\r\n",
    [ISOTP_N_BS_TIMEOUT]                    = "ISO-TP error: N_Bs timeout waiting for flow control frame\r\n",
    [ISOTP_INVALID_FLOW_CONTROL_FRAME]      = "ISO-TP error: received frame is not a flow control frame\r\n",
    [ISOTP_FLOW_CONTROL_FRAME_MAX_EXCEEDED] = "ISO-TP error: exceeded max number of flow control frames\r\n",
    [ISOTP_FLOW_CONTROL_OVERFLOW_ABORT]     = "ISO-TP error: flow control frame signaled overflow/abort\r\n",
    [ISOTP_INVALID_FLOW_CONTROL_STATUS]     = "ISO-TP error: flow control frame has an invalid status\r\n",
};

void IsoTpError(ISOTPERROR error){
    UARTSend((uint8_t *)errorMessages[error], 0, true);
}
