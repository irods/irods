/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See generalRowPurge.h for a description of this API call.*/

#include "generalRowPurge.h"
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
rsGeneralRowPurge (rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp )
{
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog(LOG_DEBUG, "generalRowPurge");

    status = getAndConnRcatHost(rsComm, MASTER_RCAT, NULL, &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
       status = _rsGeneralRowPurge (rsComm, generalRowPurgeInp);
#else
       status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
       status = rcGeneralRowPurge(rodsServerHost->conn,
			       generalRowPurgeInp);
    }

    if (status < 0) { 
       rodsLog (LOG_NOTICE,
		"rsGeneralRowPurge: rcGeneralRowPurge failed");
    }
    return (status);
}

#ifdef RODS_CAT
int
_rsGeneralRowPurge(rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp )
{
    int status;

    rodsLog (LOG_DEBUG,
	     "_rsGeneralRowPurge tableName=%s", 
	     generalRowPurgeInp->tableName);

    if (strcmp(generalRowPurgeInp->tableName,"serverload")==0) {
       status = chlPurgeServerLoad(rsComm, 
				 generalRowPurgeInp->secondsAgo);
       return(status);
    }
    if (strcmp(generalRowPurgeInp->tableName,"serverloaddigest")==0) {
       status = chlPurgeServerLoadDigest(rsComm, 
				 generalRowPurgeInp->secondsAgo);
       return(status);
    }
    return(CAT_INVALID_ARGUMENT);
} 
#endif
