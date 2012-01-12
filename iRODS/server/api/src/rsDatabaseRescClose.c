/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See databaseRescClose.h for a description of this API call.*/

#include "databaseRescClose.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "dboHighLevelRoutines.h"
int
remoteDatabaseRescClose(rsComm_t *rsComm,
		      databaseRescCloseInp_t *databaseRescCloseInp,
		      rodsServerHost_t *rodsServerHost) {
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDatabaseClose: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcDatabaseRescClose(rodsServerHost->conn,
			       databaseRescCloseInp);
    return status;
}

int
rsDatabaseRescClose (rsComm_t *rsComm, databaseRescCloseInp_t *databaseRescCloseInp)
{
    rodsServerHost_t *rodsServerHost;
    int status;
    int remoteFlag;
    rodsHostAddr_t rescAddr;
    rescGrpInfo_t *rescGrpInfo = NULL;

    status = _getRescInfo (rsComm, databaseRescCloseInp->dbrName, &rescGrpInfo);
    if (status < 0) {
	 rodsLog (LOG_ERROR,
		  "rsDatabaseRescClose: _getRescInfo of %s error, stat = %d",
		  databaseRescCloseInp->dbrName, status);
	return status;
    }

    bzero (&rescAddr, sizeof (rescAddr));
    rstrcpy (rescAddr.hostAddr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
    remoteFlag = resolveHost (&rescAddr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
       status = _rsDatabaseRescClose(rsComm, databaseRescCloseInp);
    } else if (remoteFlag == REMOTE_HOST) {
       status = remoteDatabaseRescClose(rsComm,
				      databaseRescCloseInp, 
				      rodsServerHost );
    } else if (remoteFlag < 0) {
       status = remoteFlag;
    }

    if (status < 0 ) { 
        rodsLog (LOG_NOTICE,
		 "rsDatabaseRescClose: rcDatabaseRescClose failed, status = %d",
		 status);
    }
    return (status);
}

int
_rsDatabaseRescClose (rsComm_t *rsComm, databaseRescCloseInp_t *databaseRescCloseInp)
{

#ifdef DBR
    int status;
    status = dbrClose(databaseRescCloseInp->dbrName);
    if (status < 0 ) { 
       rodsLog (LOG_NOTICE, 
		"_rsDatabaseRescClose: databaseRescClose for %s, status = %d",
		databaseRescCloseInp->dbrName, status);
    }
    return (status);
#else
    return(DBR_NOT_COMPILED_IN);
#endif
} 

