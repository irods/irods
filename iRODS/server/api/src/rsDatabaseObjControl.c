/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See databaseObjControl.h for a description of this API call.*/

#include "databaseObjControl.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "dboHighLevelRoutines.h"

int
remoteDatabaseObjControl(rsComm_t *rsComm,
		      databaseObjControlInp_t *databaseObjControlInp,
		      databaseObjControlOut_t **databaseObjControlOut,
		      rodsServerHost_t *rodsServerHost) {
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDatabaseControl: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
	*databaseObjControlOut = (databaseObjControlOut_t*)malloc(sizeof(databaseObjControlOut_t));
	memset (*databaseObjControlOut, 0, sizeof (databaseObjControlOut_t));
        return status;
    }

    status = rcDatabaseObjControl(rodsServerHost->conn,
			       databaseObjControlInp, databaseObjControlOut);
    return status;
}

int
rsDatabaseObjControl (rsComm_t *rsComm, databaseObjControlInp_t *databaseObjControlInp,
		      databaseObjControlOut_t **databaseObjControlOut)
{
    rodsServerHost_t *rodsServerHost;
    int status;
    int remoteFlag;
    rodsHostAddr_t rescAddr;
    rescGrpInfo_t *rescGrpInfo = NULL;

    status = _getRescInfo (rsComm, databaseObjControlInp->dbrName, &rescGrpInfo);
    if (status < 0) {
	 rodsLog (LOG_ERROR,
		  "rsDatabaseObjControl: _getRescInfo of %s error, stat = %d",
		  databaseObjControlInp->dbrName, status);
	char *outBuf;
	databaseObjControlOut_t *myObjControlOut;
	outBuf = (char*)malloc(200);
	*outBuf='\0';
	strncpy(outBuf, "DBR not found",200);
	myObjControlOut = (databaseObjControlOut_t*)malloc(sizeof(databaseObjControlOut_t));
	memset (myObjControlOut, 0, sizeof (databaseObjControlOut_t));
	myObjControlOut->outBuf = outBuf;
	*databaseObjControlOut = myObjControlOut;
	return status;
    }

    bzero (&rescAddr, sizeof (rescAddr));
    rstrcpy (rescAddr.hostAddr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
    remoteFlag = resolveHost (&rescAddr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
       status = _rsDatabaseObjControl(rsComm, databaseObjControlInp,
				   databaseObjControlOut);
    } else if (remoteFlag == REMOTE_HOST) {
       status = remoteDatabaseObjControl(rsComm,
				      databaseObjControlInp, 
				      databaseObjControlOut,
				      rodsServerHost );
    } else if (remoteFlag < 0) {
       status = remoteFlag;
    }

    if (status < 0 ) { 
        rodsLog (LOG_NOTICE,
		 "rsDatabaseObjControl: rcDatabaseObjControl failed, status = %d",
		 status);
    }
    return (status);
}


int
_rsDatabaseObjControl (rsComm_t *rsComm, 
		       databaseObjControlInp_t *databaseObjControlInp,
		       databaseObjControlOut_t **databaseObjControlOut) {
    char *outBuf;
    databaseObjControlOut_t *myObjControlOut;
#ifdef DBR
    int status;
    int maxBufSize;
    int validOption=0;

    maxBufSize = 1024*50;

    outBuf = (char *)malloc(maxBufSize);
    *outBuf='\0';

    if (databaseObjControlInp->option == DBO_EXECUTE) {
       validOption=1;
       status = dboExecute(rsComm, 
			   databaseObjControlInp->dbrName, 
			   databaseObjControlInp->dboName, 
			   databaseObjControlInp->dborName, 
			   databaseObjControlInp->subOption,
			   outBuf, maxBufSize, 
			   databaseObjControlInp->args);
    }
    if (databaseObjControlInp->option == DBR_COMMIT) {
       validOption=1;
       status = dbrCommit(rsComm, 
			  databaseObjControlInp->dbrName);
    }
    if (databaseObjControlInp->option == DBR_ROLLBACK) {
       validOption=1;
       status = dbrRollback(rsComm, 
			  databaseObjControlInp->dbrName);
    }

    if (validOption==0) {
       return(DBO_INVALID_CONTROL_OPTION);
    }

    myObjControlOut = (databaseObjControlOut_t *)malloc(sizeof(databaseObjControlOut_t));
    memset (myObjControlOut, 0, sizeof (databaseObjControlOut_t));
    myObjControlOut->outBuf = outBuf;

    *databaseObjControlOut = myObjControlOut;

    if (status < 0 ) { 
       rodsLog (LOG_NOTICE, 
		"_rsDatabaseObjControl: databaseObjControl status = %d",
		status);
       return (status);
    }

    return (status);
#else
    myObjControlOut = (databaseObjControlOut_t*)malloc(sizeof(databaseObjControlOut_t));
    memset (myObjControlOut, 0, sizeof (databaseObjControlOut_t));
    outBuf = (char*)malloc(100);
    strcpy(outBuf, 
	   "The iRODS system needs to be re-compiled with DBR support enabled");
    myObjControlOut->outBuf = outBuf;
    *databaseObjControlOut = myObjControlOut;
    return(DBR_NOT_COMPILED_IN);
#endif
}
