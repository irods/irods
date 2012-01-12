/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See databaseRescOpen.h for a description of this API call.*/

#include "databaseRescOpen.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "dboHighLevelRoutines.h"
int
remoteDatabaseRescOpen(rsComm_t *rsComm,
		      databaseRescOpenInp_t *databaseRescOpenInp,
		      rodsServerHost_t *rodsServerHost) {
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDatabaseOpen: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcDatabaseRescOpen(rodsServerHost->conn,
			       databaseRescOpenInp);
    return status;
}

int
rsDatabaseRescOpen (rsComm_t *rsComm, databaseRescOpenInp_t *databaseRescOpenInp)
{
    rodsServerHost_t *rodsServerHost;
    int status;
    int remoteFlag;
    rodsHostAddr_t rescAddr;
    rescGrpInfo_t *rescGrpInfo = NULL;

    status = _getRescInfo (rsComm, databaseRescOpenInp->dbrName, &rescGrpInfo);
    if (status < 0) {
	 rodsLog (LOG_ERROR,
		  "rsDatabaseRescOpen: _getRescInfo of %s error, stat = %d",
		  databaseRescOpenInp->dbrName, status);
	return status;
    }

    bzero (&rescAddr, sizeof (rescAddr));
    rstrcpy (rescAddr.hostAddr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
    remoteFlag = resolveHost (&rescAddr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
       status = _rsDatabaseRescOpen(rsComm, databaseRescOpenInp);
    } else if (remoteFlag == REMOTE_HOST) {
       status = remoteDatabaseRescOpen(rsComm,
				      databaseRescOpenInp, 
				      rodsServerHost );
    } else if (remoteFlag < 0) {
       status = remoteFlag;
    }

    if (status < 0 ) { 
        rodsLog (LOG_NOTICE,
		 "rsDatabaseRescOpen: rcDatabaseRescOpen failed, status = %d",
		 status);
    }
    return (status);
}

int
_rsDatabaseRescOpen (rsComm_t *rsComm, databaseRescOpenInp_t *databaseRescOpenInp)
{

#ifdef DBR
    int status;
    status = dbrOpen(databaseRescOpenInp->dbrName);
    if (status < 0 ) { 
       rodsLog (LOG_NOTICE, 
		"_rsDatabaseRescOpen: databaseRescOpen for %s, status = %d",
		databaseRescOpenInp->dbrName, status);
    }
    return (status);
#else
    return(DBR_NOT_COMPILED_IN);
#endif
} 

