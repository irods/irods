/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataGet.h for a description of this API call.*/

#include "dataGet.h"

int
rcDataGet (rcComm_t *conn, dataOprInp_t *dataGetInp,
portalOprOut_t **portalOprOut)
{
    int status;
    status = procApiRequest (conn, DATA_GET_AN, dataGetInp, NULL, 
        (void **) portalOprOut, NULL);

    return (status);
}
