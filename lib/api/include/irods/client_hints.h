#ifndef CLIENT_HINTS_H__
#define CLIENT_HINTS_H__

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"


#ifdef __cplusplus
extern "C"
#endif
int rcClientHints(rcComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
