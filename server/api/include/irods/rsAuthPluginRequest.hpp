#ifndef RS_AUTH_PLUGIN_REQUEST_HPP
#define RS_AUTH_PLUGIN_REQUEST_HPP

#include "irods/rcConnect.h"
#include "irods/authPluginRequest.h"

int rsAuthPluginRequest(rsComm_t* server_comm_ptr, authPluginReqInp_t* incoming_struct_with_scheme, authPluginReqOut_t** response_from_agent);

#endif
