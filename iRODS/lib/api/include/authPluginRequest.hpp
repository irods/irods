


#ifndef __AUTH_PLUGIN_REQUEST_HPP__
#define __AUTH_PLUGIN_REQUEST_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "authRequest.hpp"


struct authPluginReqInp_t {
    char auth_scheme_[ MAX_NAME_LEN ];
    char context_    [ MAX_NAME_LEN ];

}; // struct authPluginReqInp_t
#define authPlugReqInp_PI "str auth_scheme_[MAX_NAME_LEN]; str context_[MAX_NAME_LEN];"

struct authPluginReqOut_t {
    char result_[ MAX_NAME_LEN ];

}; // struct authPluginReqOut_t
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

// =-=-=-=-=-=-=-
// prototype for client
int rcAuthPluginRequest(
    rcComm_t*,              // server comm ptr
    authPluginReqInp_t*,    // incoming struct with scheme
    authPluginReqOut_t** ); // response from agent

#endif // __AUTH_PLUGIN_REQUEST_HPP__



