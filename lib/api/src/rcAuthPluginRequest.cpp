// =-=-=-=-=-=-=-
#include "authPluginRequest.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int rcAuthPluginRequest(
    rcComm_t*             _comm,
    authPluginReqInp_t*   _req_inp,
    authPluginReqOut_t**  _req_out ) {
    int status = procApiRequest(
                     _comm,
                     AUTH_PLUG_REQ_AN,
                     _req_inp,
                     nullptr,
                     ( void ** )_req_out,
                     nullptr );
    return status;

} // rcAuthPluginRequest



