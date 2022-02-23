#ifndef RS_AUTH_CHECK_HPP
#define RS_AUTH_CHECK_HPP

#include "irods/authCheck.h"

int rsAuthCheck( rsComm_t *rsComm, authCheckInp_t *authCheckInp, authCheckOut_t **authCheckOut );

#endif
