#ifndef RS_AUTH_REQUEST_HPP
#define RS_AUTH_REQUEST_HPP

#include "authRequest.h"

int rsAuthRequest( rsComm_t *rsComm, authRequestOut_t **authRequestOut );
char * _rsAuthRequestGetChallenge();


#endif
