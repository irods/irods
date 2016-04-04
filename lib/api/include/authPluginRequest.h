#ifndef AUTH_PLUGIN_REQUEST_H__
#define AUTH_PLUGIN_REQUEST_H__

// =-=-=-=-=-=-=-
// irods includes
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

// =-=-=-=-=-=-=-
// prototype for server
#if defined(RODS_SERVER)
#define RS_AUTH_PLUG_REQ rsAuthPluginRequest
int rsAuthPluginRequest(
    rsComm_t*,              // server comm ptr
    authPluginReqInp_t*,    // incoming struct with scheme
    authPluginReqOut_t** ); // response from agent
#else
#define RS_AUTH_PLUG_REQ NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
// =-=-=-=-=-=-=-
// prototype for client
int rcAuthPluginRequest(
    rcComm_t*,              // server comm ptr
    authPluginReqInp_t*,    // incoming struct with scheme
    authPluginReqOut_t** ); // response from agent
#ifdef __cplusplus
}
#endif

#endif // AUTH_PLUGIN_REQUEST_H__
