#ifndef RS_PAM_AUTH_REQUEST_HPP
#define RS_PAM_AUTH_REQUEST_HPP

#include "irods/pamAuthRequest.h"

int rsPamAuthRequest( rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp, pamAuthRequestOut_t **pamAuthRequestOut );
int _rsPamAuthRequest( rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp, pamAuthRequestOut_t **pamAuthRequestOut );

#endif
