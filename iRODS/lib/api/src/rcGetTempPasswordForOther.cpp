/* This is script-generated code.  */ 
/* See getTempPasswordForOther.h for a description of this API call.*/

#include "getTempPasswordForOther.h"

int
rcGetTempPasswordForOther (rcComm_t *conn, 
		   getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
		   getTempPasswordForOtherOut_t **getTempPasswordForOtherOut)
{
    int status;
    status = procApiRequest (conn, GET_TEMP_PASSWORD_FOR_OTHER_AN,  getTempPasswordForOtherInp, NULL, 
        (void **) getTempPasswordForOtherOut, NULL);

    return (status);
}
