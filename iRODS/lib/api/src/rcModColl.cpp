/* This is script-generated code.  */ 
/* See modColl.h for a description of this API call.*/

#include "modColl.h"

int
rcModColl (rcComm_t *conn, collInp_t *modCollInp)
{
    int status;
    status = procApiRequest (conn, MOD_COLL_AN, modCollInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
