/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileOpr.c - File type operation. Will call low level file drivers
 */

#include "fileOpr.h"
#include "fileStat.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int
initFileDesc ()
{
    memset (FileDesc, 0, sizeof (fileDesc_t) * NUM_FILE_DESC);
    return (0);
}

int
allocFileDesc ()
{
    int i;

    for (i = 3; i < NUM_FILE_DESC; i++) {
	if (FileDesc[i].inuseFlag == FD_FREE) {
	    FileDesc[i].inuseFlag = FD_INUSE;
	    return (i);
	};
    }

    rodsLog (LOG_NOTICE,
     "allocFileDesc: out of FileDesc");

    return (SYS_OUT_OF_FILE_DESC);
}

int
allocAndFillFileDesc (rodsServerHost_t *rodsServerHost, char *fileName,
fileDriverType_t fileType, int fd, int mode)
{
    int fileInx;

    fileInx = allocFileDesc ();
    if (fileInx < 0) {
	return (fileInx);
    }

    FileDesc[fileInx].rodsServerHost = rodsServerHost;
    FileDesc[fileInx].fileName = strdup (fileName);
    FileDesc[fileInx].fileType = fileType;
    FileDesc[fileInx].mode = mode;
    FileDesc[fileInx].fd = fd;

    return (fileInx);
}

int
freeFileDesc (int fileInx)
{
    if (fileInx < 3 || fileInx >= NUM_FILE_DESC) {
	rodsLog (LOG_NOTICE,
	 "freeFileDesc: fileInx %d out of range", fileInx); 
	return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    if (FileDesc[fileInx].fileName != NULL) {
	free (FileDesc[fileInx].fileName);
    }

    /* don't free driverDep (dirPtr is not malloced */

    memset (&FileDesc[fileInx], 0, sizeof (fileDesc_t));

    return (0);
} 

int
getServerHostByFileInx (int fileInx, rodsServerHost_t **rodsServerHost)
{
    int remoteFlag;

    if (fileInx < 3 || fileInx >= NUM_FILE_DESC) {
        rodsLog (LOG_NOTICE,
          "getServerHostByFileInx: Bad fileInx value %d", fileInx);
        return (SYS_BAD_FILE_DESCRIPTOR);
    }

    if (FileDesc[fileInx].inuseFlag == 0) {
        rodsLog (LOG_NOTICE,
          "getServerHostByFileInx: fileInx %d not active", fileInx);
        return (SYS_BAD_FILE_DESCRIPTOR);
    }

    *rodsServerHost = FileDesc[fileInx].rodsServerHost;
    remoteFlag = (*rodsServerHost)->localFlag;

    return (remoteFlag);
}

int
mkDirForFilePath (int fileType, rsComm_t *rsComm, char *startDir,
char *filePath, int mode)
{
    int status;
 
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if ((status = splitPathByKey (filePath, myDir, myFile, '/')) < 0) {
        rodsLog (LOG_NOTICE,
	  "mkDirForFilePath: splitPathByKey for %s error, status = %d", 
	  filePath, status);
	return (status);
    }

    status = mkFileDirR (fileType, rsComm, startDir, myDir, mode);

    return (status);
}

/* mk the directory resursively */ 

int
mkFileDirR (int fileType, rsComm_t *rsComm, char *startDir, 
char *destDir, int mode)
{
    int status;
    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];
    struct stat statbuf;

    startLen = strlen (startDir);
    pathLen = strlen (destDir);

    rstrcpy (tmpPath, destDir, MAX_NAME_LEN);

    tmpLen = pathLen;

    while (tmpLen > startLen) {
        status = fileStat ( (fileDriverType_t)fileType, rsComm, tmpPath, &statbuf);
        if (status >= 0) {
            if (statbuf.st_mode & S_IFDIR) {
                break;
            } else {
		 rodsLog (LOG_NOTICE,
                 "mkFileDirR: A local non-directory %s already exists \n",
                  tmpPath);
                return (status);
            }
        }

        /* Go backward */

        while (tmpLen && tmpPath[tmpLen] != '/')
            tmpLen --;
        tmpPath[tmpLen] = '\0';
    }

    /* Now we go forward and make the required dir */
    while (tmpLen < pathLen) {
        /* Put back the '/' */
        tmpPath[tmpLen] = '/';
       status = fileMkdir ((fileDriverType_t)fileType, rsComm, tmpPath, mode);
        if (status < 0) {
	    rodsLog (LOG_NOTICE,
             "mkFileDirR: mkdir failed for %s, status =%d",
              tmpPath, status);
            return status;
        }
#if 0	/* a fix from AndyS */
        while (tmpLen && tmpPath[tmpLen] != '\0')
#endif
        while (tmpPath[tmpLen] != '\0')
            tmpLen ++;
    }
    return 0;
}

int
chkEmptyDir (int fileType, rsComm_t *rsComm, char *cacheDir)
{
    void *dirPtr = NULL;
#if defined(solaris_platform)
    char fileDirent[sizeof (struct dirent) + MAX_NAME_LEN];
    struct dirent *myFileDirent = (struct dirent *) fileDirent;
#else
    struct dirent fileDirent;
    struct dirent *myFileDirent = &fileDirent;
#endif
    int status;
    char childPath[MAX_NAME_LEN];
    struct stat myFileStat;

    status = fileOpendir ((fileDriverType_t)fileType, rsComm, cacheDir, &dirPtr);

    if (status < 0) {
        return (0);
    }

    while ((status = fileReaddir ((fileDriverType_t)fileType, rsComm, dirPtr, myFileDirent))
      >= 0) {
        if (strcmp (myFileDirent->d_name, ".") == 0 ||
          strcmp (myFileDirent->d_name, "..") == 0) {
            continue;
        }
        snprintf (childPath, MAX_NAME_LEN, "%s/%s", cacheDir, 
	  myFileDirent->d_name);

        status = fileStat ((fileDriverType_t)fileType, rsComm, childPath, &myFileStat);

        if (status < 0) {
            rodsLog (LOG_ERROR,
              "chkEmptyDir: fileStat error for %s, status = %d",
              childPath, status);
	    break;
        }
	if (myFileStat.st_mode & S_IFREG) {
            rodsLog (LOG_ERROR,
              "chkEmptyDir: file %s exists",
              childPath, status);
	    status = SYS_DIR_IN_VAULT_NOT_EMPTY;
            break;
        }

	if (myFileStat.st_mode & S_IFDIR) {
	    status = chkEmptyDir ((fileDriverType_t)fileType, rsComm, childPath);
	    if (status == SYS_DIR_IN_VAULT_NOT_EMPTY) {
                rodsLog (LOG_ERROR,
                  "chkEmptyDir: dir %s is not empty", childPath);
	        break;
	    }
	}
    }
    fileClosedir ((fileDriverType_t)fileType, rsComm, dirPtr);
    if (status != SYS_DIR_IN_VAULT_NOT_EMPTY) {
	fileRmdir ((fileDriverType_t)fileType, rsComm, cacheDir);
	status = 0;
    }
    return status;
}

/* chkFilePathPerm - check the FilePath permission.
 * If rodsServerHost
 */
int
chkFilePathPerm (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (strstr (fileOpenInp->fileName, "/../") != NULL) {
        /* don't allow /../ in the path */
	rodsLog (LOG_ERROR,
	  "chkFilePathPerm: input fileName %s contains /../",
	  fileOpenInp->fileName);
        return SYS_INVALID_FILE_PATH;
    }

    if (rodsServerHost == NULL) {
	rodsLog (LOG_NOTICE,
	  "chkFilePathPerm: NULL rodsServerHost");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    status = matchCliVaultPath (rsComm, fileOpenInp->fileName, 
      rodsServerHost);

    if (status > 0) {
	/* a match in vault */
	return (status);
    }

    status = rsChkNVPathPermByHost (rsComm, fileOpenInp, rodsServerHost);
    
    return (status);
}

int
matchVaultPath (rsComm_t *rsComm, char *filePath, 
rodsServerHost_t *rodsServerHost, char **outVaultPath)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int len;

    if (strstr (filePath, "/../") != NULL) {
	/* no match */
	return (0);
    }
    tmpRescGrpInfo = RescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
	tmpRescInfo = tmpRescGrpInfo->rescInfo;
	/* match the rodsServerHost */
	if (tmpRescInfo->rodsServerHost == rodsServerHost) {
	    len = strlen (tmpRescInfo->rescVaultPath);
	    if (strncmp (tmpRescInfo->rescVaultPath, filePath, len) == 0) {
		*outVaultPath = tmpRescInfo->rescVaultPath;
		return (len);
	    }
	}
	tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    /* no match */
    return (0);
}

int
matchCliVaultPath (rsComm_t *rsComm, char *filePath,
rodsServerHost_t *rodsServerHost)
{
    int len, nameLen;
    char *tmpPath;
    char *outVaultPath = NULL;


    if ((len = 
      matchVaultPath (rsComm, filePath, rodsServerHost, &outVaultPath)) == 0) {
	/* no match */
	return (0);
    }

    /* assume the path is $(vaultPath/myzone/home/userName or 
     * $(vaultPath/home/userName */

    nameLen = strlen (rsComm->clientUser.userName);

    tmpPath = filePath + len + 1;    /* skip the vault path */

    if ((tmpPath = strchr (tmpPath, '/')) == NULL) return 0;
    tmpPath++;
    if (strncmp (tmpPath, rsComm->clientUser.userName, nameLen) == 0) 
	return 1;

    if ((tmpPath = strchr (tmpPath, '/')) == NULL) return 0;
    tmpPath++;
    if (strncmp (tmpPath, rsComm->clientUser.userName, nameLen) == 0) 
        return 1;
    else
	return 0;

#if 0
    /* skip two more '/' */

    count = 0;
    while (*tmpPath != '\0') {
	if (*tmpPath == '/') {
	    count++;
	    if (count >= 2) {
		tmpPath++;
		break;
	    }
	}
	tmpPath++;
    }

    if (count < 2) {
	return (0);
    }
 
    len = strlen (rsComm->clientUser.userName);
    if (strncmp (tmpPath, rsComm->clientUser.userName, len) == 0) {
	return (1);
    } else {
	return (0);
    }
#endif
}

/* filePathTypeInResc - the status of a filePath in a resource.
 * return LOCAL_FILE_T, LOCAL_DIR_T, UNKNOWN_OBJ_T or error (-ive)
 */ 
int
filePathTypeInResc (rsComm_t *rsComm, char *fileName, rescInfo_t *rescInfo)
{
    int rescTypeInx;
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    int status;

    memset (&fileStatInp, 0, sizeof (fileStatInp));

    rstrcpy (fileStatInp.fileName, fileName, MAX_NAME_LEN);

    rescTypeInx = rescInfo->rescTypeInx;
    fileStatInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileStatInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
    status = rsFileStat (rsComm, &fileStatInp, &myStat);

    if (status < 0) return status;
    if (myStat->st_mode & S_IFREG) {
	free (myStat);
	return LOCAL_FILE_T;
    } else if (myStat->st_mode & S_IFDIR) {
	free (myStat);
	return LOCAL_DIR_T;
    } else {
	free (myStat);
	return UNKNOWN_OBJ_T;
    }
}

int
bindStreamToIRods (rodsServerHost_t *rodsServerHost, int fd)
{
    int fileInx;

    fileInx = allocAndFillFileDesc (rodsServerHost, STREAM_FILE_NAME, 
      UNIX_FILE_TYPE, fd, DEFAULT_FILE_MODE);

    return fileInx;
}

