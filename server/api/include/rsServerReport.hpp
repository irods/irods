#ifndef RS_SERVER_REPORT_HPP
#define RS_SERVER_REPORT_HPP

#include "rodsConnect.h"
#include "objInfo.h"

int rsServerReport(rsComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
