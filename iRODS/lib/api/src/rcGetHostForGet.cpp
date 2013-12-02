/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See getHostForGet.h for a description of this API call.*/

#include "getHostForGet.h"

int
rcGetHostForGet (rcComm_t *conn, dataObjInp_t *dataObjInp,
char **outHost)
{
    int status;
    status = procApiRequest (conn, GET_HOST_FOR_GET_AN,  dataObjInp, NULL, 
        (void **) outHost, NULL);

    return (status);
}

