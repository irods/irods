/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileRmdir.h for a description of this API call.*/

#include "fileRmdir.h"
#include "fileOpendir.h"
#include "fileReaddir.h"
#include "fileClosedir.h"
#include "miscServerFunct.h"
#include "tarSubStructFileDriver.h"

int
rsFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileRmdirInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileRmdir (rsComm, fileRmdirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileRmdir (rsComm, fileRmdirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileRmdir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileRmdir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileRmdir (rodsServerHost->conn, fileRmdirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileOpen: rcFileRmdir failed for %s",
          fileRmdirInp->dirName);
    }

    return status;
}

int
_rsFileRmdir (rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp)
{
    int status;

    if ((fileRmdirInp->flags & RMDIR_RECUR) != 0) {
	/* recursive. This is a very dangerous operation. curently
	 * it is only used to remove cache directory of structured 
	 * files */
	void *dirPtr = NULL;
#if defined(solaris_platform)
        char fileDirent[sizeof (struct dirent) + MAX_NAME_LEN];
        struct dirent *myFileDirent = (struct dirent *) fileDirent;
#else
        struct dirent fileDirent;
        struct dirent *myFileDirent = &fileDirent;
#endif

	if (strstr (fileRmdirInp->dirName, CACHE_DIR_STR) == NULL) {
            rodsLog (LOG_ERROR,
              "_rsFileRmdir: recursive rm of non cachedir path %s",
              fileRmdirInp->dirName);
	    return (SYS_INVALID_FILE_PATH);
	}

        status = fileOpendir (fileRmdirInp->fileType, rsComm,
          fileRmdirInp->dirName, &dirPtr);

        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "_rsFileRmdir: fileOpendir for %s, status = %d",
              fileRmdirInp->dirName, status);
            return (status);
        }

	while ((status = fileReaddir (fileRmdirInp->fileType, rsComm,
	  dirPtr, myFileDirent)) >= 0) {
	    struct stat statbuf;
	    char myPath[MAX_NAME_LEN];
	
            if (strcmp (myFileDirent->d_name, ".") == 0 ||
              strcmp (myFileDirent->d_name, "..") == 0) {
                continue;
            }

            snprintf (myPath, MAX_NAME_LEN, "%s/%s",
                  fileRmdirInp->dirName, myFileDirent->d_name);

	    status = fileStat (fileRmdirInp->fileType, rsComm, 
	      myPath, &statbuf);
	    if (status < 0) {
                rodsLog (LOG_NOTICE,
                  "_rsFileRmdir: fileStat for %s, status = %d",
                  myPath, status);
		fileClosedir (fileRmdirInp->fileType, rsComm, dirPtr);
		return (status);
	    } 
	    if ((statbuf.st_mode & S_IFREG) != 0) {     /* A file */
		status = fileUnlink (fileRmdirInp->fileType, rsComm, myPath);
	    } else if ((statbuf.st_mode & S_IFDIR) != 0) {
		/* A directory */
		fileRmdirInp_t myRmdirInp;
		memset (&myRmdirInp, 0, sizeof (myRmdirInp));
		myRmdirInp.fileType = fileRmdirInp->fileType;
		myRmdirInp.flags = fileRmdirInp->flags;
		rstrcpy (myRmdirInp.dirName, myPath, MAX_NAME_LEN);
		status = _rsFileRmdir (rsComm, &myRmdirInp);
	    } else {
                rodsLog (LOG_NOTICE,
                  "_rsFileRmdir:  for %s, status = %d",
                  myPath, status);

	    }
	    if (status < 0) {
		rodsLog (LOG_NOTICE,
                  "_rsFileRmdir:  rm of %s failed, status = %d",
                  myPath, status);
		fileClosedir (fileRmdirInp->fileType, rsComm, dirPtr);
		return (status);
	    }
	}
	fileClosedir (fileRmdirInp->fileType, rsComm, dirPtr);
	if (status < 0 && status != -1) {	/* end of file */
	    return (status);
	}
    }
    status = fileRmdir (fileRmdirInp->fileType, rsComm, fileRmdirInp->dirName);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileRmdir: fileRmdir for %s, status = %d",
          fileRmdirInp->dirName, status);
        return (status);
    }

    return (status);
} 
