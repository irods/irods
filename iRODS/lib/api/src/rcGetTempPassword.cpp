/* This is script-generated code.  */ 
/* See getTempPassword.h for a description of this API call.*/

#include "getTempPassword.h"

int
rcGetTempPassword (rcComm_t *conn, getTempPasswordOut_t **getTempPasswordOut)
{
    int status;
    status = procApiRequest (conn, GET_TEMP_PASSWORD_AN, NULL, NULL, 
        (void **) getTempPasswordOut, NULL);

    return (status);
}
