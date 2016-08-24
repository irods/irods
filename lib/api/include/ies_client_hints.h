#ifndef IES_CLIENT_HINTS_H__
#define IES_CLIENT_HINTS_H__

#include "rodsDef.h"
#include "rcConnect.h"

#define IES_CLIENT_HINTS_AN 10216

#ifdef __cplusplus
extern "C"
#endif
int rcIESClientHints(rcComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
