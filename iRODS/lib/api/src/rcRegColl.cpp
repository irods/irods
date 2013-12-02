/* This is script-generated code.  */ 
/* See regColl.h for a description of this API call.*/

#include "regColl.h"

int
rcRegColl (rcComm_t *conn, collInp_t *regCollInp)
{
    int status;
    status = procApiRequest (conn, REG_COLL_AN, regCollInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
