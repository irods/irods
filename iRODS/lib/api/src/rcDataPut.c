/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataPut.h for a description of this API call.*/

#include "dataPut.h"

int
rcDataPut (rcComm_t *conn, dataOprInp_t *dataPutInp,
portalOprOut_t **portalOprOut)
{
    int status;
    status = procApiRequest (conn, DATA_PUT_AN,  dataPutInp, NULL, 
        (void **) portalOprOut, NULL);

    return (status);
}
