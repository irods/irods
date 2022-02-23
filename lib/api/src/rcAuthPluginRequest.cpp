// =-=-=-=-=-=-=-
#include "irods/authPluginRequest.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int rcAuthPluginRequest(
    rcComm_t*             _comm,
    authPluginReqInp_t*   _req_inp,
    authPluginReqOut_t**  _req_out ) {
    int status = procApiRequest(
                     _comm,
                     AUTH_PLUG_REQ_AN,
                     _req_inp,
                     NULL,
                     ( void ** )_req_out,
                     NULL );
    return status;

} // rcAuthPluginRequest



