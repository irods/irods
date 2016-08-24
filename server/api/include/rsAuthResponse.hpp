#ifndef RS_AUTH_RESPONSE_HPP
#define RS_AUTH_RESPONSE_HPP

#include "rcConnect.h"
#include "authResponse.h"

int rsAuthResponse( rsComm_t *rsComm, authResponseInp_t *authResponseInp );
int chkProxyUserPriv( rsComm_t *rsComm, int proxyUserPriv );

#endif
