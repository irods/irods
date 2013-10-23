


#ifndef __AUTH_PLUGIN_REQUEST_H__
#define __AUTH_PLUGIN_REQUEST_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "authRequest.h"


struct authPluginReqInp_t {
    char auth_scheme_[ NAME_LEN ];

}; // struct authPluginReqInp_t
#define authPlugReqInp_PI "str auth_scheme_[NAME_LEN];"

// =-=-=-=-=-=-=-
// prototype for server
#if defined(RODS_SERVER)
    #define RS_AUTH_PLUG_REQ rsAuthPluginRequest
    int rsAuthPluginRequest(
        rsComm_t*,             // server comm ptr
        authPluginReqInp_t*,   // incoming struct with scheme
        authRequestOut_t** ); // outoing struct with challenge & user/zone
#else
#define RS_AUTH_PLUG_REQ NULL
#endif

// =-=-=-=-=-=-=-
// prototype for client
int rcAuthPluginRequest(
    rcComm_t*,            // server comm ptr
    authPluginReqInp_t*,  // incoming struct with scheme
    authRequestOut_t** ); // outoing struct with challenge & user/zone

#endif // __AUTH_PLUGIN_REQUEST_H__



