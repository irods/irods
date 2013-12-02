/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See endTransaction.h for a description of this API call.*/

#include "endTransaction.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsEndTransaction (rsComm_t *rsComm, endTransactionInp_t *endTransactionInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog(LOG_DEBUG, "endTransaction");

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsEndTransaction (rsComm, endTransactionInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcEndTransaction(rodsServerHost->conn,
                            endTransactionInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
                "rsEndTransaction: rcEndTransaction failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsEndTransaction(rsComm_t *rsComm, endTransactionInp_t *endTransactionInp )
{
   int status;

   rodsLog (LOG_DEBUG,
	    "_rsEndTransaction arg0=%s", 
	    endTransactionInp->arg0);

   if (strcmp(endTransactionInp->arg0,"commit")==0) {
      status = chlCommit(rsComm);
      return(status);
   }

   if (strcmp(endTransactionInp->arg0,"rollback")==0) {
      status = chlRollback(rsComm);
      return(status);
   }

   return(CAT_INVALID_ARGUMENT);
}
#endif
