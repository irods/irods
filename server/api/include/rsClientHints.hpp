#ifndef RS_CLIENT_HINTS_HPP
#define RS_CLIENT_HINTS_HPP

#include "rodsDef.h"
#include "rcConnect.h"

int rsClientHints(rsComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
