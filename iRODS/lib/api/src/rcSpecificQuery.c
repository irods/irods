/* This is script-generated code.  */ 
/* See specificQuery.h for a description of this API call.*/

#include "specificQuery.h"

int
rcSpecificQuery (rcComm_t *conn, specificQueryInp_t *specificQueryInp, 
genQueryOut_t **genQueryOut)
{
    int status;
    status = procApiRequest (conn, SPECIFIC_QUERY_AN,  specificQueryInp, NULL, 
        (void **)genQueryOut, NULL);

    return (status);
}
