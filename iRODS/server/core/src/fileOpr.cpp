/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* fileOpr.c - File type operation. Will call low level file drivers
 */

#include "fileOpr.hpp"
#include "fileStat.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "collection.hpp"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.hpp"
#include "eirods_file_object.hpp"
#include "eirods_collection_object.hpp"
#include "eirods_stacktrace.hpp"
#include "eirods_resource_backport.hpp"
#include "eirods_resource_manager.hpp"

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
allocAndFillFileDesc (rodsServerHost_t *rodsServerHost, char* objPath, char *fileName,
                      char* rescHier, int fd, int mode)
{
    int fileInx;

    fileInx = allocFileDesc ();
    if (fileInx < 0) {
        return (fileInx);
    }

    FileDesc[fileInx].rodsServerHost = rodsServerHost;
    FileDesc[fileInx].objPath  = strdup (objPath);
    FileDesc[fileInx].fileName = strdup (fileName);
    FileDesc[fileInx].rescHier = strdup (rescHier);
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

    int startLen;
    int pathLen, tmpLen;
    char tmpPath[MAX_NAME_LEN];
    struct stat statbuf;

    startLen = strlen (startDir);
    pathLen = strlen (destDir);

    rstrcpy (tmpPath, destDir, MAX_NAME_LEN);

    tmpLen = pathLen;

    while (tmpLen > startLen) {
        eirods::collection_object_ptr tmp_coll_obj( new eirods::collection_object( tmpPath, eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE, 0, 0 ) );
        eirods::error stat_err = fileStat( rsComm, tmp_coll_obj, &statbuf );
        if ( stat_err.code() >= 0) {
            if (statbuf.st_mode & S_IFDIR) {
                break;
            } else {
                rodsLog (LOG_NOTICE,
                         "mkFileDirR: A local non-directory %s already exists \n",
                         tmpPath);
                return (stat_err.code() );
            }
        } else {
            // debug code only -- eirods::log( stat_err );
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
     
        eirods::collection_object_ptr tmp_coll_obj( new eirods::collection_object( tmpPath, eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE, mode, 0 ) ); 
        eirods::error mkdir_err = fileMkdir( rsComm, tmp_coll_obj );
        if ( !mkdir_err.ok() && (getErrno ( mkdir_err.code()) != EEXIST)) { // JMC - backport 4834
            std::stringstream msg;
            msg << "fileMkdir for [";
            msg << tmpPath;
            msg << "]";
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
int chkEmptyDir (rsComm_t *rsComm, char *cacheDir) {
    
    int status;
    char childPath[MAX_NAME_LEN];
    struct stat myFileStat;
    struct rodsDirent* myFileDirent = 0;

    // =-=-=-=-=-=-=-
    // call opendir via resource plugin
    eirods::collection_object_ptr cacheDir_obj( new eirods::collection_object( cacheDir, eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE, 0, 0 ) );
    eirods::error opendir_err = fileOpendir( rsComm, cacheDir_obj );

    // =-=-=-=-=-=-=-
    // dont log an error as this is part
    // of the functionality of this code
    if( !opendir_err.ok() ) {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // make call to readdir via resource plugin
    eirods::error readdir_err = fileReaddir( rsComm, cacheDir_obj, &myFileDirent );
    while( readdir_err.ok() && 0 == readdir_err.code() ) {
        // =-=-=-=-=-=-=-
        // handle relative paths
        if( strcmp( myFileDirent->d_name, "." ) == 0 ||
            strcmp( myFileDirent->d_name, "..") == 0) {
            readdir_err = fileReaddir( rsComm, cacheDir_obj, &myFileDirent );
            continue;
        }

        // =-=-=-=-=-=-=-
        // get status of path
        snprintf( childPath, MAX_NAME_LEN, "%s/%s", cacheDir, myFileDirent->d_name );
        eirods::collection_object_ptr tmp_coll_obj( new eirods::collection_object( childPath, eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE, 0, 0 ) );
        eirods::error stat_err = fileStat( rsComm, tmp_coll_obj, &myFileStat );

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
            status = chkEmptyDir( rsComm, childPath);
            if (status == SYS_DIR_IN_VAULT_NOT_EMPTY) {
                rodsLog( LOG_ERROR, "chkEmptyDir: dir %s is not empty", childPath );
                break;
            }
        }
                
        // =-=-=-=-=-=-=-
        // continue with child path
        readdir_err = fileReaddir( rsComm, cacheDir_obj, &myFileDirent );

    } // while

    // =-=-=-=-=-=-=-
    // make call to closedir via resource plugin, log error if necessary
    eirods::error closedir_err = fileClosedir( rsComm, cacheDir_obj );
    if( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "fileClosedir failed for [";
        msg << cacheDir;
        msg << "]";
        eirods::error log_err = PASSMSG( msg.str(), closedir_err );
        eirods::log( log_err ); 
    }

    if( status != SYS_DIR_IN_VAULT_NOT_EMPTY ) {
        eirods::collection_object_ptr coll_obj( new eirods::collection_object( cacheDir, eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE, 0, 0 ) );
        eirods::error rmdir_err = fileRmdir( rsComm, coll_obj );
        if( !rmdir_err.ok() ) {
            std::stringstream msg;
            msg << "fileRmdir failed for [";
            msg << cacheDir;
            msg << "]";
            eirods::error err = PASSMSG( msg.str(), rmdir_err );
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
        std::string out_path;
        eirods::error ret = resc_mgr.validate_vault_path( fileOpenInp->fileName, rodsServerHost, out_path );
        if( ret.ok() ) {
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

/* matchCliVaultPath - if the input path is inside 
 * $(vaultPath)/home/userName, return 1.
 * If it is in $(vaultPath) but not in $(vaultPath)/home/userName,
 * return -1. If it is a non vault path, return 0.
 */
int matchCliVaultPath( 
    rsComm_t*          _comm, 
    const std::string& _path,
    rodsServerHost_t*  _svr_host ) {
    // =-=-=-=-=-=-=-
    // sanity check
    if( !_comm ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: null comm" );
        return SYS_INVALID_INPUT_PARAM;
    }

    if( _path.empty() ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: empty file path" );
        return SYS_INVALID_INPUT_PARAM;
    }

    if( !_svr_host ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: null server host" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // validate the vault path against a known host
    std::string vault_path;
    eirods::error ret = resc_mgr.validate_vault_path( _path, _svr_host, vault_path );
    if( !ret.ok() || vault_path.empty() ) {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // get the substr of the file path, minus the vault path
    std::string user_path = _path.substr( vault_path.size() );
    
    // =-=-=-=-=-=-=-
    // if user_path starts with a / set the home pos accordingly
    const size_t home_pos = ( user_path[0] == '/' ) ? 1 : 0;

    // =-=-=-=-=-=-=-
    // verify that the first chars are home/
    size_t pos = user_path.find( "home/" );
    if( home_pos != pos ) {
        rodsLog( LOG_NOTICE, "matchCliVaultPath :: home/ is not found in the proper location for path [%s]", 
                 user_path.c_str() );
        return -1; 
    }
 
    // =-=-=-=-=-=-=-
    // if user_path starts with a / set the home pos accordingly
    const size_t user_pos = home_pos + 5;

    // =-=-=-=-=-=-=-
    // verify that the user name follows 'home/'
    pos = user_path.find( _comm->clientUser.userName );
    if( user_pos != pos ) {
        rodsLog( LOG_NOTICE, "matchCliVaultPath :: [%s] is not found in the proper location for path [%s]", 
                 _comm->clientUser.userName, user_path.c_str() );
        return -1; 
    }

    // =-=-=-=-=-=-=-
    // success
    return 1;

#if 0
    int len, nameLen;
    char *tmpPath;
    char *outVaultPath = NULL;


    if ((len = 
         matchVaultPath (rsComm, filePath, rodsServerHost, &outVaultPath)) == 0) {
        /* no match */
        return (0);
    }

    /* assume the path is $(vaultPath/home/userName */

    nameLen = strlen (rsComm->clientUser.userName);

    tmpPath = filePath + len + 1;    /* skip the vault path */

    if (strncmp (tmpPath, "home/", 5) != 0) return -1;
    tmpPath += 5;
    if( strncmp (tmpPath, rsComm->clientUser.userName, nameLen) == 0 &&
        (tmpPath[nameLen] == '/' || tmpPath[len] == '\0') )
        return 1;
    else
        return 0;

    return -1;
#endif
} // matchCliVaultPath

/* filePathTypeInResc - the status of a filePath in a resource.
 * return LOCAL_FILE_T, LOCAL_DIR_T, UNKNOWN_OBJ_T or error (-ive)
 */ 
int
filePathTypeInResc (
    rsComm_t*   rsComm,
    char*       objPath,
    char*       fileName,
    char*       rescHier,
    rescInfo_t* rescInfo ) {
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    int status;

    // =-=-=-=-=-=-=-
    // get location of leaf in hier string
    std::string location;
    eirods::error ret = eirods::get_loc_for_hier_string( rescHier, location );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "failed in get_loc_for_hier_string", ret ) );
        return -1;
    }
    
    memset (&fileStatInp, 0, sizeof (fileStatInp));
    rstrcpy (fileStatInp.fileName, fileName, MAX_NAME_LEN);
    rstrcpy (fileStatInp.rescHier, rescHier, MAX_NAME_LEN);
    rstrcpy (fileStatInp.objPath,  objPath,  MAX_NAME_LEN);
    rstrcpy (fileStatInp.addr.hostAddr,  location.c_str(), NAME_LEN);
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

    fileInx = allocAndFillFileDesc (rodsServerHost, "", STREAM_FILE_NAME, "",
                                    fd, DEFAULT_FILE_MODE);

    return fileInx;
}

