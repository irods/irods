#ifndef RS_AUTHENTICATE_HPP
#define RS_AUTHENTICATE_HPP

#include "irods/rcConnect.h"
#include "irods/authenticate.h"

int rsAuthenticate( rsComm_t *rsComm, AuthenticateInp_t *authenticateInp, AuthenticateOut_t **authenticateOut );
int _rsAuthenticate( rsComm_t *rsComm, AuthenticateInp_t *authenticateInp, AuthenticateOut_t **authenticateOut );

#endif
