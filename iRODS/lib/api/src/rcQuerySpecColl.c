/* This is script-generated code.  */ 
/* See querySpecColl.h for a description of this API call.*/

#include "querySpecColl.h"

int
rcQuerySpecColl (rcComm_t *conn, dataObjInp_t *querySpecCollInp,
genQueryOut_t **genQueryOut)
{
    int status;
    status = procApiRequest (conn, QUERY_SPEC_COLL_AN, querySpecCollInp, NULL, 
        (void **) genQueryOut, NULL);

    return (status);
}
