#ifndef AUTH_PLUGIN_REQUEST_H__
#define AUTH_PLUGIN_REQUEST_H__

#include "rcConnect.h"

typedef struct AuthPluginReqInp {
    char auth_scheme_[ MAX_NAME_LEN ];
    char context_    [ MAX_NAME_LEN ];
} authPluginReqInp_t;
#define authPlugReqInp_PI "str auth_scheme_[MAX_NAME_LEN]; str context_[MAX_NAME_LEN];"

typedef struct AuthPluginReqOut {
    char result_[ MAX_NAME_LEN ];
} authPluginReqOut_t;
#define authPlugReqOut_PI "str result_[MAX_NAME_LEN];"


#ifdef __cplusplus
extern "C"
#endif
int rcAuthPluginRequest(rcComm_t* server_comm_ptr, authPluginReqInp_t* incoming_struct_with_scheme, authPluginReqOut_t** response_from_agent);

#endif
