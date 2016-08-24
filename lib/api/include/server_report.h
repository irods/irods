#ifndef SERVER_REPORT_H__
#define SERVER_REPORT_H__

#include "rcConnect.h"
#include "objInfo.h"

#ifdef __cplusplus
extern "C"
#endif
int rcServerReport(rcComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
