/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See simpleQuery.h for a description of this API call.*/

#include "simpleQuery.h"

int
rcSimpleQuery (rcComm_t *conn, simpleQueryInp_t *simpleQueryInp, 
simpleQueryOut_t **simpleQueryOut)
{
    int status;
    status = procApiRequest (conn, SIMPLE_QUERY_AN,  simpleQueryInp, NULL, 
        (void **) simpleQueryOut, NULL);

    return (status);
}
