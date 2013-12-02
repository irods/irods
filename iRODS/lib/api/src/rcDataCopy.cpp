/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataCopy.h for a description of this API call.*/

#include "dataCopy.h"

int
rcDataCopy (rcComm_t *conn, dataCopyInp_t *dataCopyInp)
{
    int status;
    status = procApiRequest (conn, DATA_COPY_AN, dataCopyInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
