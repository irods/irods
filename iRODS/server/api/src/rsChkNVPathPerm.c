/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See chkNVPathPerm.h for a description of this API call.*/

#include "chkNVPathPerm.h"
#include "fileStat.h"
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsChkNVPathPerm (rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&chkNVPathPermInp->addr, &rodsServerHost);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else {
        status = rsChkNVPathPermByHost (rsComm, chkNVPathPermInp,
          rodsServerHost);
        return (status);
    }
}

int
rsChkNVPathPermByHost (rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp,
rodsServerHost_t *rodsServerHost)
{
    int remoteFlag;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
         "rsChkNVPathPermByHost: Input NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    remoteFlag = rodsServerHost->localFlag;

    if (remoteFlag == LOCAL_HOST) {
        status = _rsChkNVPathPerm (rsComm, chkNVPathPermInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteChkNVPathPerm (rsComm, chkNVPathPermInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsChkNVPathPerm: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteChkNVPathPerm (rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteChkNVPathPerm: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcChkNVPathPerm (rodsServerHost->conn, chkNVPathPermInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteChkNVPathPerm: rcChkNVPathPerm failed for %s",
          chkNVPathPermInp->fileName);
    }

    return status;
}

int
_rsChkNVPathPerm (rsComm_t *rsComm, fileOpenInp_t *chkNVPathPermInp)
{
    int status;
    struct stat myFileStat;
    int sysUid;
    char userName[NAME_LEN], tmpPath[MAX_NAME_LEN];
    int len;
    char *tmpPtr;

    /* Need to match path's owner uid with sysUid */

    sysUid = rsComm->clientUser.sysUid;
    if (sysUid < 0) {
        /* have tried before */
        return (SYS_NO_PATH_PERMISSION);
    } else if (sysUid == 0) {
	if (strstr (rsComm->clientUser.userName, "@") != NULL) {
            splitPathByKey (rsComm->clientUser.userName, 
	      userName, tmpPath, '@');
	} else {
	    rstrcpy (userName, rsComm->clientUser.userName, NAME_LEN);
	}
	sysUid = rsComm->clientUser.sysUid = getUnixUid (userName);
	if (sysUid < 0) {
	    rsComm->clientUser.sysUid = sysUid;
	    return (SYS_NO_PATH_PERMISSION);
	}
    }

    
    rstrcpy (tmpPath, chkNVPathPermInp->fileName, MAX_NAME_LEN);

    len = strlen (tmpPath);

    status = -1;
    while (1) {
        status = fileStat (chkNVPathPermInp->fileType, rsComm, 
         tmpPath, &myFileStat);
	if (status >= 0) {
	    break;
	} else if (errno == EEXIST || getErrno (status) == EEXIST) {

	    /* go back */
	    tmpPtr =  tmpPath + len;

	    while (len > 0) {
		len --;
		if (*tmpPtr == '/') {
		    *tmpPtr = '\0';
		    break;
		}
		tmpPtr--;
	    }

	    if (len > 0) {
		/* give it more tries */
		continue;
	    } else {
		break;
	    }
	} else {
	    break;
	}
    }
	    
    if (status < 0) {
	return (SYS_NO_PATH_PERMISSION);
    }
	    
    if (sysUid != (int) myFileStat.st_uid && 
      (myFileStat.st_mode & S_IWOTH) == 0) {
	return (SYS_NO_PATH_PERMISSION);
    } else {
        return (0);
    }
} 
