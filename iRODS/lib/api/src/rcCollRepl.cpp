/* This is script-generated code.  */ 
/* See collRepl.h for a description of this API call.*/

#include "collRepl.h"

int
_rcCollRepl (rcComm_t *conn, collInp_t *collReplInp, 
collOprStat_t **collOprStat)
{
    int status;

    collReplInp->oprType = REPLICATE_OPR;

    status = procApiRequest (conn, COLL_REPL_AN, collReplInp, NULL, 
        (void **) collOprStat, NULL);

    return status;
}

int
rcCollRepl (rcComm_t *conn, collInp_t *collReplInp, int vFlag)
{
    int status, retval;
    collOprStat_t *collOprStat = NULL;

    retval = _rcCollRepl (conn, collReplInp, &collOprStat);

    status = cliGetCollOprStat (conn, collOprStat, vFlag, retval);

    return (status);
}

