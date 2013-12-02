/* This is script-generated code.  */ 
/* See chkObjPermAndStat.h for a description of this API call.*/

#include "chkObjPermAndStat.h"

int
rcChkObjPermAndStat (rcComm_t *conn, chkObjPermAndStat_t *chkObjPermAndStatInp)
{
    int status;
    status = procApiRequest (conn, CHK_OBJ_PERM_AND_STAT_AN, 
      chkObjPermAndStatInp, NULL, (void **) NULL, NULL);

    return (status);
}
