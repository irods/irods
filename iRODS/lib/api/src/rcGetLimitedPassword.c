/* This is script-generated code.  */ 
/* See getLimitedPassword.h for a description of this API call.*/

#include "getLimitedPassword.h"

int
rcGetLimitedPassword (rcComm_t *conn, 
		      getLimitedPasswordInp_t *getLimitedPasswordInp,
		      getLimitedPasswordOut_t **getLimitedPasswordOut)
{
    int status;
    status = procApiRequest (conn, GET_LIMITED_PASSWORD_AN, 
			     getLimitedPasswordInp, NULL, 
			     (void **) getLimitedPasswordOut, NULL);
    return (status);
}
