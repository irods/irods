#ifndef RS_CLIENT_HINTS_HPP
#define RS_CLIENT_HINTS_HPP

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"

int rsClientHints(rsComm_t* server_comm_ptr, bytesBuf_t** json_response);

#endif
