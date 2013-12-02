/* This is script-generated code.  */ 
/* See procStat.h for a description of this API call.*/

#include "procStat.h"

int
rcProcStat (rcComm_t *conn, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut)
{
    int status;
    status = procApiRequest (conn, PROC_STAT_AN, procStatInp, 
      NULL, (void **) procStatOut, NULL);

    return (status);
}
