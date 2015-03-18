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
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_manager.hpp"
#include "irods_resource_plugin.hpp"

int
initFileDesc() {
    memset( FileDesc, 0, sizeof( FileDesc ) );
    return 0;
}

int
allocFileDesc() {
    int i;

    for ( i = 3; i < NUM_FILE_DESC; i++ ) {
        if ( FileDesc[i].inuseFlag == FD_FREE ) {
            FileDesc[i].inuseFlag = FD_INUSE;
            return i;
        };
    }

    rodsLog( LOG_NOTICE,
             "allocFileDesc: out of FileDesc" );

    return SYS_OUT_OF_FILE_DESC;
}

int
allocAndFillFileDesc( rodsServerHost_t *rodsServerHost,
                      const std::string&  objPath,
                      const std::string&  fileName,
                      const std::string&  rescHier,
                      int                 fd,
                      int                 mode ) {
    int fileInx;

    fileInx = allocFileDesc();
    if ( fileInx < 0 ) {
        return fileInx;
    }

    FileDesc[fileInx].rodsServerHost = rodsServerHost;
    FileDesc[fileInx].objPath  = strdup( objPath.c_str() );
    FileDesc[fileInx].fileName = strdup( fileName.c_str() );
    FileDesc[fileInx].rescHier = strdup( rescHier.c_str() );
    FileDesc[fileInx].mode = mode;
    FileDesc[fileInx].fd = fd;

    return fileInx;
}

int
freeFileDesc( int fileInx ) {
    if ( fileInx < 3 || fileInx >= NUM_FILE_DESC ) {
        rodsLog( LOG_NOTICE,
                 "freeFileDesc: fileInx %d out of range", fileInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    if ( FileDesc[fileInx].fileName != NULL ) {
        free( FileDesc[fileInx].fileName );
    }

    if ( FileDesc[fileInx].objPath != NULL ) {
        free( FileDesc[fileInx].objPath );
    }

    if ( FileDesc[fileInx].rescHier != NULL ) {
        free( FileDesc[fileInx].rescHier );
    }

    /* don't free driverDep (dirPtr is not malloced */

    memset( &FileDesc[fileInx], 0, sizeof( fileDesc_t ) );

    return 0;
}

int
getServerHostByFileInx( int fileInx, rodsServerHost_t **rodsServerHost ) {
    int remoteFlag;
    if ( fileInx < 3 || fileInx >= NUM_FILE_DESC ) {
        rodsLog( LOG_DEBUG,
                 "getServerHostByFileInx: Bad fileInx value %d", fileInx );
        return SYS_BAD_FILE_DESCRIPTOR;
    }

    if ( FileDesc[fileInx].inuseFlag == 0 ) {
        rodsLog( LOG_DEBUG,
                 "getServerHostByFileInx: fileInx %d not active", fileInx );
        return SYS_BAD_FILE_DESCRIPTOR;
    }

    *rodsServerHost = FileDesc[fileInx].rodsServerHost;
    remoteFlag = ( *rodsServerHost )->localFlag;

    return remoteFlag;
}

int
mkDirForFilePath(
    rsComm_t *          rsComm,
    size_t              startDirLen,
    const std::string&  filePath,
    const std::string&  hier,
    int                 mode ) {
    int status;

    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if ( ( status = splitPathByKey( filePath.c_str(), myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_NOTICE,
                 "mkDirForFilePath: splitPathByKey for %s error, status = %d",
                 filePath.c_str(), status );
        return status;
    }

    status = mkFileDirR( rsComm, startDirLen, myDir, hier, mode );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "mkFileDirR failed in mkDirForFilePath with status %d", status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// mk the directory recursively
int mkFileDirR(
    rsComm_t *          rsComm,
    size_t              startDirLen,
    const std::string&  destDir,
    const std::string&  hier,
    int                 mode ) {

    std::string physical_directory_prefix;
    if ( destDir.empty() ) {
        rodsLog( LOG_ERROR, "mkFileDirR called with empty dest directory" );
        return SYS_INVALID_INPUT_PARAM ;
    }
    if ( destDir.size() < startDirLen ) {
        rodsLog( LOG_ERROR, "mkFileDirR called with a destDir: [%s]"
                 "shorter than its startDirLen: [%ju]",
                 destDir.c_str(), ( uintmax_t )startDirLen );
        return SYS_INVALID_INPUT_PARAM;
    }
    if ( !rsComm ) {
        rodsLog( LOG_ERROR, "mkFileDirR called with null rsComm" );
        return SYS_INVALID_INPUT_PARAM;
    }
    if ( isValidFilePath( destDir ) ) {
        std::string vault_path;
        irods::error err = irods::get_vault_path_for_hier_string( hier, vault_path );
        if ( !err.ok() ) {
            rodsLog( LOG_ERROR, err.result().c_str() );
            return err.code();
        }

        if ( destDir.compare( 0, vault_path.size(), vault_path ) == 0 &&
                ( destDir[ vault_path.size() ] == '/' || destDir.size() == vault_path.size() ) ) {
            physical_directory_prefix = vault_path;
        }
    }

    std::vector< std::string > directories_to_create;

    std::string physical_directory = destDir;
    if ( physical_directory[ physical_directory.size() - 1 ] == '/' ) {
        physical_directory.erase( physical_directory.size() - 1 );
    }
    while ( physical_directory.size() > startDirLen ) {
        irods::collection_object_ptr tmp_coll_obj(
            new irods::collection_object(
                physical_directory,
                hier,
                0, 0 ) );
        struct stat statbuf;
        irods::error stat_err = fileStat(
                                    rsComm,
                                    tmp_coll_obj,
                                    &statbuf );
        if ( stat_err.code() >= 0 ) {
            if ( statbuf.st_mode & S_IFDIR ) {
                break;
            }
            else {
                rodsLog( LOG_NOTICE,
                         "mkFileDirR: A local non-directory %s already exists \n",
                         physical_directory.c_str() );
                return stat_err.code();
            }
        }
        else {
            directories_to_create.push_back( physical_directory.substr( physical_directory_prefix.size() ) );
        }

        /* Go backward */
        size_t index_of_last_slash = physical_directory.rfind( '/', physical_directory.size() - 1 );
        if ( std::string::npos != index_of_last_slash ) {
            physical_directory = physical_directory.substr( 0, index_of_last_slash );
        }
        else {
            break;
        }

    } // while

    std::string irods_directory_prefix = "/";
    irods_directory_prefix += getLocalZoneName();

    /* Now we go forward and make the required dir */
    while ( !directories_to_create.empty() ) {

        physical_directory = physical_directory_prefix;
        physical_directory += directories_to_create.back();

        std::string irods_directory = irods_directory_prefix;
        irods_directory += directories_to_create.back();

        directories_to_create.pop_back();

        irods::collection_object_ptr tmp_coll_obj(
            new irods::collection_object(
                physical_directory,
                hier,
                mode, 0 ) );

        irods::error mkdir_err = fileMkdir( rsComm, tmp_coll_obj );
        if ( !mkdir_err.ok() && ( getErrno( mkdir_err.code() ) != EEXIST ) ) { // JMC - backport 4834
            std::stringstream msg;
            msg << "fileMkdir for [";
            msg << physical_directory;
            msg << "]";
            irods::error ret_err = PASSMSG( msg.str(), mkdir_err );
            irods::log( ret_err );

            return  mkdir_err.code();
        }
    }
    return 0;
}

// =-=-=-=-=-=-=-
//
int chkEmptyDir(
    rsComm_t*           rsComm,
    const std::string&  cacheDir,
    const std::string&  hier ) {
    int status = 0;
    char childPath[MAX_NAME_LEN];
    struct stat myFileStat;
    struct rodsDirent* myFileDirent = 0;

    // =-=-=-=-=-=-=-
    // call opendir via resource plugin
    irods::collection_object_ptr cacheDir_obj(
        new irods::collection_object(
            cacheDir,
            hier,
            0, 0 ) );
    irods::error opendir_err = fileOpendir( rsComm, cacheDir_obj );

    // =-=-=-=-=-=-=-
    // don't log an error as this is part
    // of the functionality of this code
    if ( !opendir_err.ok() ) {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // make call to readdir via resource plugin
    irods::error readdir_err = fileReaddir( rsComm, cacheDir_obj, &myFileDirent );
    while ( readdir_err.ok() && 0 == readdir_err.code() ) {
        // =-=-=-=-=-=-=-
        // handle relative paths
        if ( strcmp( myFileDirent->d_name, "." ) == 0 ||
                strcmp( myFileDirent->d_name, ".." ) == 0 ) {
            readdir_err = fileReaddir( rsComm, cacheDir_obj, &myFileDirent );
            continue;
        }

        // =-=-=-=-=-=-=-
        // get status of path
        snprintf( childPath, MAX_NAME_LEN, "%s/%s", cacheDir.c_str(), myFileDirent->d_name );
        irods::collection_object_ptr tmp_coll_obj(
            new irods::collection_object(
                childPath,
                hier,
                0, 0 ) );

        irods::error stat_err = fileStat( rsComm, tmp_coll_obj, &myFileStat );

        // =-=-=-=-=-=-=-
        // handle hard error
        if ( stat_err.code() < 0 ) {
            rodsLog( LOG_ERROR, "chkEmptyDir: fileStat error for %s, status = %d",
                     childPath, stat_err.code() );
            break;
        }

        // =-=-=-=-=-=-=-
        // path exists
        if ( myFileStat.st_mode & S_IFREG ) {
            status = SYS_DIR_IN_VAULT_NOT_EMPTY;
            rodsLog( LOG_ERROR, "chkEmptyDir: file %s exists",
                     childPath, status );
            break;
        }

        // =-=-=-=-=-=-=-
        // recurse for another directory
        if ( myFileStat.st_mode & S_IFDIR ) {
            status = chkEmptyDir( rsComm, childPath, hier );
            if ( status == SYS_DIR_IN_VAULT_NOT_EMPTY ) {
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
    irods::error closedir_err = fileClosedir( rsComm, cacheDir_obj );
    if ( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "fileClosedir failed for [";
        msg << cacheDir;
        msg << "]";
        irods::error log_err = PASSMSG( msg.str(), closedir_err );
        irods::log( log_err );
    }

    if ( status != SYS_DIR_IN_VAULT_NOT_EMPTY ) {
        irods::collection_object_ptr coll_obj(
            new irods::collection_object(
                cacheDir,
                hier, 0, 0 ) );
        irods::error rmdir_err = fileRmdir( rsComm, coll_obj );
        if ( !rmdir_err.ok() ) {
            std::stringstream msg;
            msg << "fileRmdir failed for [";
            msg << cacheDir;
            msg << "]";
            irods::error err = PASSMSG( msg.str(), rmdir_err );
            irods::log( err );
        }
        status = 0;
    }

    return status;

} // chkEmptyDir

/* chkFilePathPerm - check the FilePath permission.
 * If rodsServerHost
 */
int
chkFilePathPerm( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
                 rodsServerHost_t *rodsServerHost, int chkType ) { // JMC - backport 4774
    int status;

    if ( chkType == NO_CHK_PATH_PERM ) {
        return 0;
    }
    else if ( chkType == DISALLOW_PATH_REG ) {
        return PATH_REG_NOT_ALLOWED;
    }
    // =-=-=-=-=-=-=-

    status = isValidFilePath( fileOpenInp->fileName ); // JMC - backport 4766
    if ( status < 0 ) {
        return status;    // JMC - backport 4766
    }


    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "chkFilePathPerm: NULL rodsServerHost" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    if ( chkType == CHK_NON_VAULT_PATH_PERM ) {
        status = matchCliVaultPath( rsComm, fileOpenInp->fileName, rodsServerHost );

        if ( status == 1 ) {
            /* a match in vault */
            return status;
        }
        else if ( status == -1 ) {
            /* in vault, but not in user's vault */
            return CANT_REG_IN_VAULT_FILE;
        }
    }
    else if ( chkType == DO_CHK_PATH_PERM ) {
        std::string out_path;
        irods::error ret = resc_mgr.validate_vault_path( fileOpenInp->fileName, rodsServerHost, out_path );
        if ( ret.ok() ) {
            /* a match */
            return CANT_REG_IN_VAULT_FILE;
        }
    }
    else {
        return SYS_INVALID_INPUT_PARAM;
        // =-=-=-=-=-=-=-
    }

    status = rsChkNVPathPermByHost( rsComm, fileOpenInp, rodsServerHost );

    return status;
}

// =-=-=-=-=-=-=-
// JMC - backport 4766, 4869
/* isValidFilePath - check if it is a valid file path - should not contain
 * "/../" or end with "/.."
 */
int
isValidFilePath( const std::string& path ) {

    if ( path.find( "/../" ) != std::string::npos ||
            path.compare( path.size() - 3, path.size(), "/.." ) == 0 ) {
        /* "/../" or end with "/.."  */
        rodsLog( LOG_ERROR, "isValidFilePath: inp fileName %s contains /../ or ends with /..", path.c_str() );
        return SYS_INVALID_FILE_PATH;
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
    if ( !_comm ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: null comm" );
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( _path.empty() ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: empty file path" );
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( !_svr_host ) {
        rodsLog( LOG_ERROR, "matchCliVaultPath :: null server host" );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // validate the vault path against a known host
    std::string vault_path;
    irods::error ret = resc_mgr.validate_vault_path( _path, _svr_host, vault_path );
    if ( !ret.ok() || vault_path.empty() ) {
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
    if ( home_pos != pos ) {
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
    if ( user_pos != pos ) {
        rodsLog( LOG_NOTICE, "matchCliVaultPath :: [%s] is not found in the proper location for path [%s]",
                 _comm->clientUser.userName, user_path.c_str() );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // success
    return 1;

} // matchCliVaultPath

/* filePathTypeInResc - the status of a filePath in a resource.
 * return LOCAL_FILE_T, LOCAL_DIR_T, UNKNOWN_OBJ_T or error (-ive)
 */
int
filePathTypeInResc(
    rsComm_t*           rsComm,
    const std::string&  objPath,
    const std::string&  fileName,
    const std::string&  rescHier ) {
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    int status;

    // =-=-=-=-=-=-=-
    // get location of leaf in hier string
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    memset( &fileStatInp, 0, sizeof( fileStatInp ) );
    rstrcpy( fileStatInp.fileName, fileName.c_str(), MAX_NAME_LEN );
    rstrcpy( fileStatInp.rescHier, rescHier.c_str(), MAX_NAME_LEN );
    rstrcpy( fileStatInp.objPath,  objPath.c_str(),  MAX_NAME_LEN );
    rstrcpy( fileStatInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    status = rsFileStat( rsComm, &fileStatInp, &myStat );

    if ( status < 0 || NULL == myStat ) {
        return status;    // JMC cppcheck - nullptr
    }
    if ( myStat->st_mode & S_IFREG ) {
        free( myStat );
        return LOCAL_FILE_T;
    }
    else if ( myStat->st_mode & S_IFDIR ) {
        free( myStat );
        return LOCAL_DIR_T;
    }
    else {
        free( myStat );
        return UNKNOWN_OBJ_T;
    }
}

int
bindStreamToIRods( rodsServerHost_t *rodsServerHost, int fd ) {
    int fileInx;

    fileInx = allocAndFillFileDesc( rodsServerHost, "", STREAM_FILE_NAME, "",
                                    fd, DEFAULT_FILE_MODE );

    return fileInx;
}

