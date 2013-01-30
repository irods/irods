/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileOpr.c - File type operation. Will call low level file drivers
 */

#include "fileOpr.h"
#include "fileStat.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"

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
                      char* rescHier, fileDriverType_t fileType, int fd, int mode)
{
    int fileInx;

    fileInx = allocFileDesc ();
    if (fileInx < 0) {
        return (fileInx);
    }

    FileDesc[fileInx].rodsServerHost = rodsServerHost;
    FileDesc[fileInx].fileName = strdup (fileName);
    FileDesc[fileInx].rescHier = strdup (rescHier);
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

    if(FileDesc[fileInx].rescHier != NULL) {
        free (FileDesc[fileInx].rescHier);
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
mkDirForFilePath (
    rsComm_t *rsComm,
    const char *startDir,
    const char *filePath,
    int mode)
{
    int status;

    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if ((status = splitPathByKey (filePath, myDir, myFile, '/')) < 0) {
        rodsLog (LOG_NOTICE,
                 "mkDirForFilePath: splitPathByKey for %s error, status = %d", 
                 filePath, status);
        return (status);
    }

    status = mkFileDirR (rsComm, startDir, myDir, mode);

    return (status);
}

// =-=-=-=-=-=-=-
// mk the directory resursively 
int mkFileDirR(
    rsComm_t *rsComm,
    const char *startDir,
    const char *destDir,
    int mode ) {

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
        eirods::collection_object tmp_coll_obj( tmpPath, 0, 0 );
        eirods::error stat_err = fileStat( tmp_coll_obj, &statbuf );
        if ( stat_err.code() >= 0) {
            if (statbuf.st_mode & S_IFDIR) {
                break;
            } else {
                rodsLog (LOG_NOTICE,
                         "mkFileDirR: A local non-directory %s already exists \n",
                         tmpPath);
                return (stat_err.code() );
            }
        }

        /* Go backward */
        while (tmpLen && tmpPath[tmpLen] != '/')
            tmpLen --;

        tmpPath[tmpLen] = '\0';

    } // while


    /* Now we go forward and make the required dir */
    while (tmpLen < pathLen) {
        /* Put back the '/' */
        tmpPath[tmpLen] = '/';
     
        eirods::collection_object tmp_coll_obj( tmpPath, mode, 0 ); 
        eirods::error mkdir_err = fileMkdir( tmp_coll_obj );
        if ( !mkdir_err.ok() && (getErrno ( mkdir_err.code()) != EEXIST)) { // JMC - backport 4834
            std::stringstream msg;
            msg << "mkFileDirR: fileMkdir for ";
            msg << tmpPath;
            msg << ", status = ";
            msg <<  mkdir_err.code();
            eirods::error ret_err = PASSMSG( msg.str(), mkdir_err );
            eirods::log( ret_err );

            return  mkdir_err.code();
        }
        while (tmpPath[tmpLen] != '\0')
            tmpLen ++;
    }
    return 0;
}

// =-=-=-=-=-=-=-
// 
int chkEmptyDir (int fileType, rsComm_t *rsComm, char *cacheDir) {
    
    int status;
    char childPath[MAX_NAME_LEN];
    struct stat myFileStat;
    struct rodsDirent* myFileDirent = 0;

    // =-=-=-=-=-=-=-
    // call opendir via resource plugin
    eirods::collection_object cacheDir_obj( cacheDir, 0, 0 );
    eirods::error opendir_err = fileOpendir( cacheDir_obj );

    // =-=-=-=-=-=-=-
    // dont log an error as this is part
    // of the functionality of this code
    if( !opendir_err.ok() ) {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // make call to readdir via resource plugin
    eirods::error readdir_err = fileReaddir( cacheDir_obj, &myFileDirent );
    while( readdir_err.ok() && 0 == readdir_err.code() ) {
        // =-=-=-=-=-=-=-
        // handle relative paths
        if( strcmp( myFileDirent->d_name, "." ) == 0 ||
            strcmp( myFileDirent->d_name, "..") == 0) {
            readdir_err = fileReaddir( cacheDir_obj, &myFileDirent );
            continue;
        }

        // =-=-=-=-=-=-=-
        // get status of path
        snprintf( childPath, MAX_NAME_LEN, "%s/%s", cacheDir, myFileDirent->d_name );
        eirods::collection_object tmp_coll_obj( childPath, 0, 0 );
        eirods::error stat_err = fileStat( tmp_coll_obj, &myFileStat );

        // =-=-=-=-=-=-=-
        // handle hard error
        if( stat_err.code() < 0 ) {
            rodsLog( LOG_ERROR, "chkEmptyDir: fileStat error for %s, status = %d",
                     childPath, status);
            break;
        }

        // =-=-=-=-=-=-=-
        // path exists
        if( myFileStat.st_mode & S_IFREG ) {
            rodsLog( LOG_ERROR, "chkEmptyDir: file %s exists",
                     childPath, status);
            status = SYS_DIR_IN_VAULT_NOT_EMPTY;
            break;
        }

        // =-=-=-=-=-=-=-
        // recurse for another directory
        if (myFileStat.st_mode & S_IFDIR) {
            status = chkEmptyDir( -1, rsComm, childPath);
            if (status == SYS_DIR_IN_VAULT_NOT_EMPTY) {
                rodsLog( LOG_ERROR, "chkEmptyDir: dir %s is not empty", childPath );
                break;
            }
        }
                
        // =-=-=-=-=-=-=-
        // continue with child path
        readdir_err = fileReaddir( cacheDir_obj, &myFileDirent );

    } // while

    // =-=-=-=-=-=-=-
    // make call to closedir via resource plugin, log error if necessary
    eirods::error closedir_err = fileClosedir( cacheDir_obj );
    if( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "chkEmptyDir: fileClosedir for ";
        msg << cacheDir;
        msg << ", status = ";
        msg << closedir_err.code();
        eirods::error log_err = PASS( false, closedir_err.code(), msg.str(), closedir_err );
        eirods::log( log_err ); 
    }

    if( status != SYS_DIR_IN_VAULT_NOT_EMPTY ) {
        eirods::collection_object coll_obj( cacheDir, 0, 0 );
        eirods::error rmdir_err = fileRmdir( coll_obj );
        if( !rmdir_err.ok() ) {
            std::stringstream msg;
            msg << "chkEmptyDir: fileRmdir for ";
            msg << cacheDir;
            msg << ", status = ";
            msg << rmdir_err.code();
            eirods::error err = PASS( false, rmdir_err.code(), msg.str(), rmdir_err );
            eirods::log ( err );
        }
        status = 0;
    }

    return status;

} // chkEmptyDir

/* chkFilePathPerm - check the FilePath permission.
 * If rodsServerHost
 */
int
chkFilePathPerm (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
                 rodsServerHost_t *rodsServerHost, int chkType) // JMC - backport 4774
{
    int status;
    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    char *outVaultPath = NULL;

    if (chkType == NO_CHK_PATH_PERM) {
        return 0;
    } else if (chkType == DISALLOW_PATH_REG) {
        return PATH_REG_NOT_ALLOWED;
    }
    // =-=-=-=-=-=-=-

    status = isValidFilePath (fileOpenInp->fileName);  // JMC - backport 4766
    if (status < 0) return status; // JMC - backport 4766


    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "chkFilePathPerm: NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    if (chkType == CHK_NON_VAULT_PATH_PERM) {
        status = matchCliVaultPath (rsComm, fileOpenInp->fileName, rodsServerHost);

        if (status == 1) {
            /* a match in vault */
            return (status);
        } else if (status == -1) {
            /* in vault, but not in user's vault */
            return CANT_REG_IN_VAULT_FILE;
        }
    } else if (chkType == DO_CHK_PATH_PERM) {
        int ret = matchVaultPath (rsComm, fileOpenInp->fileName, rodsServerHost, &outVaultPath );
        if( ret > 0 ) {
            //if (matchVaultPath (rsComm, fileOpenInp->fileName, rodsServerHost,&outVaultPath) > 0) {
            /* a match */
            return CANT_REG_IN_VAULT_FILE;
        }
    } else {
        return SYS_INVALID_INPUT_PARAM;
        // =-=-=-=-=-=-=-
    }

    status = rsChkNVPathPermByHost (rsComm, fileOpenInp, rodsServerHost);
    
    return (status);
}

// =-=-=-=-=-=-=-
// JMC - backport 4766, 4869
/* isValidFilePath - check if it is a valid file path - should not contain
 * "/../" or end with "/.."
 */
int
isValidFilePath (char *path) 
{
    char *tmpPtr  = NULL;
    char *tmpPath = path;
#if 0
    if (strstr (path, "/../") != NULL) {
        /* don't allow /../ in the path */
        rodsLog (LOG_ERROR,
                 "isValidFilePath: input fileName %s contains /../", path);
        return SYS_INVALID_FILE_PATH;
    }
#endif
    while ((tmpPtr = strstr (tmpPath, "/..")) != NULL) {
        if (tmpPtr[3] == '\0' || tmpPtr[3] == '/') {
            /* "/../" or end with "/.."  */
            rodsLog (LOG_ERROR,"isValidFilePath: inp fileName %s contains /../ or ends with /..",path);
            return SYS_INVALID_FILE_PATH;
        } else {
            tmpPath += 3;
        }
    }
    return 0;
}
// =-=-=-=-=-=-=-
int
matchVaultPath (rsComm_t *rsComm, char *filePath, 
                rodsServerHost_t *rodsServerHost, char **outVaultPath)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int len;
    if (isValidFilePath (filePath) < 0) { // JMC - backport 4766
        /* no match */
        return (0);
    }
    tmpRescGrpInfo = RescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        /* match the rodsServerHost */
        if (tmpRescInfo->rodsServerHost == rodsServerHost) {
            len = strlen (tmpRescInfo->rescVaultPath);
            if( strncmp ( tmpRescInfo->rescVaultPath, filePath, len ) == 0 && // JMC -backport 4767
                ( filePath[len] == '/' || filePath[len] == '\0' ) ) {
                *outVaultPath = tmpRescInfo->rescVaultPath;
                return (len);
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    /* no match */
    return (0);
}

/* matchCliVaultPath - if the input path is inside 
 * $(vaultPath)/myzone/home/userName, retun 1.
 * If it is in $(vaultPath) but not in $(vaultPath)/myzone/home/userName,
 * return -1. If it is a non vault path, return 0.
 */
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

    /* assume the path is $(vaultPath/myzone/home/userName */ 

    nameLen = strlen (rsComm->clientUser.userName);

    tmpPath = filePath + len + 1;    /* skip the vault path */

    if (strncmp (tmpPath, "home/", 5) != -1) return -1;
    tmpPath += 5;
    if( strncmp (tmpPath, rsComm->clientUser.userName, nameLen) == 0 &&
        (tmpPath[nameLen] == '/' || tmpPath[len] == '\0') )
        return 1;
    else
        return 0;

    return -1;
}

/* filePathTypeInResc - the status of a filePath in a resource.
 * return LOCAL_FILE_T, LOCAL_DIR_T, UNKNOWN_OBJ_T or error (-ive)
 */ 
int
filePathTypeInResc (
    rsComm_t *rsComm,
    char *fileName,
    char *rescHier,
    rescInfo_t *rescInfo)
{
    int rescTypeInx;
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    int status;

    memset (&fileStatInp, 0, sizeof (fileStatInp));

    rstrcpy (fileStatInp.fileName, fileName, MAX_NAME_LEN);
    rstrcpy (fileStatInp.rescHier, rescHier, MAX_NAME_LEN);

    rescTypeInx = rescInfo->rescTypeInx;
    fileStatInp.fileType = static_cast< fileDriverType_t >( -1 );// JMC - legacy resource - RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileStatInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
    status = rsFileStat (rsComm, &fileStatInp, &myStat);

    if (status < 0 || NULL == myStat ) return status; // JMC cppcheck - nullptr
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

    fileInx = allocAndFillFileDesc (rodsServerHost, STREAM_FILE_NAME, "",
                                    UNIX_FILE_TYPE, fd, DEFAULT_FILE_MODE);

    return fileInx;
}

