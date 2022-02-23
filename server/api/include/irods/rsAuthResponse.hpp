#ifndef RS_AUTH_RESPONSE_HPP
#define RS_AUTH_RESPONSE_HPP

#include "irods/rcConnect.h"
#include "irods/authResponse.h"

int rsAuthResponse( rsComm_t *rsComm, authResponseInp_t *authResponseInp );
int chkProxyUserPriv( rsComm_t *rsComm, int proxyUserPriv );

#endif
