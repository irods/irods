/* This is script-generated code.  */ 
/* See oprComplete.h for a description of this API call.*/

#include "oprComplete.h"

int
rcOprComplete (rcComm_t *conn, int retval)
{
    int status;
    status = procApiRequest (conn, OPR_COMPLETE_AN, (void **) &retval, NULL, 
        (void **) NULL, NULL);

    return (status);
}
