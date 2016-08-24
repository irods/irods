/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileRmdir.h for a description of this API call.*/

#include "fileRmdir.h"
#include "fileOpendir.h"
#include "fileReaddir.h"
#include "fileClosedir.h"
#include "miscServerFunct.hpp"
#include "fileDriver.hpp"
#include "rsFileRmdir.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_collection_object.hpp"

#define CACHE_DIR_STR "cacheDir" // FIXME JMC - need a better place for this.  also used in tar plugin

int
rsFileRmdir( rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &fileRmdirInp->addr, &rodsServerHost );
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileRmdir( rsComm, fileRmdirInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileRmdir( rsComm, fileRmdirInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileRmdir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileRmdir( rsComm_t *rsComm, fileRmdirInp_t *fileRmdirInp,
                 rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileRmdir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileRmdir( rodsServerHost->conn, fileRmdirInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpen: rcFileRmdir failed for %s",
                 fileRmdirInp->dirName );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which handles removing directories via the resource plugin
int _rsFileRmdir(
    rsComm_t*       _comm,
    fileRmdirInp_t* _rmdir_inp ) {

    irods::collection_object_ptr coll_obj(
        new irods::collection_object(
            _rmdir_inp->dirName,
            _rmdir_inp->rescHier,
            0, 0 ) );

    if ( ( _rmdir_inp->flags & RMDIR_RECUR ) != 0 ) {
        // FIXME :: revisit this after moving to First class Objects
        // recursive. This is a very dangerous operation. curently
        // it is only used to remove cache directory of structured
        // files
        struct rodsDirent* myFileDirent = 0;

        // if it is not a cache dir, return an error as we only do this
        // for cache dirs at the moment.
        if ( strstr( _rmdir_inp->dirName, CACHE_DIR_STR ) == NULL ) {
            rodsLog( LOG_ERROR, "_rsFileRmdir: recursive rm of non cachedir path %s",
                     _rmdir_inp->dirName );
            return SYS_INVALID_FILE_PATH;
        }

        // call opendir via resource plugin
        irods::error opendir_err = fileOpendir( _comm, coll_obj );

        // log an error, if any
        if ( !opendir_err.ok() ) {
            std::stringstream msg;
            msg << "fileOpendir failed for [";
            msg << _rmdir_inp->dirName;
            msg << "]";
            irods::error err = PASSMSG( msg.str(), opendir_err );
            irods::log( err );
            return opendir_err.code();
        }

        // read the directory via resource plugin and either handle files or recurse into another directory
        irods::error readdir_err = fileReaddir( _comm, coll_obj, &myFileDirent );
        while ( readdir_err.ok() && 0 == readdir_err.code() ) {
            struct stat statbuf;
            char myPath[MAX_NAME_LEN];

            // handle relative paths
            if ( strcmp( myFileDirent->d_name, "." ) == 0 ||
                    strcmp( myFileDirent->d_name, ".." ) == 0 ) {
                readdir_err = fileReaddir( _comm, coll_obj, &myFileDirent );
                continue;
            }

            // cache path name
            snprintf( myPath, MAX_NAME_LEN, "%s/%s", &_rmdir_inp->dirName[0], &myFileDirent->d_name[0] );

            // =-=-=-=-=-=-=-
            // call stat via resource plugin, handle errors
            irods::collection_object_ptr myPath_obj(
                new irods::collection_object(
                    myPath,
                    _rmdir_inp->rescHier,
                    0, 0 ) );
            irods::error stat_err = fileStat( _comm, myPath_obj, &statbuf );
            if ( !stat_err.ok() ) {
                std::stringstream msg;
                msg << "fileStat failed for [";
                msg << myPath;
                msg << "]";
                irods::error log_err = PASSMSG( msg.str(), stat_err );
                irods::log( log_err );

                // call closedir via resource plugin, handle errors
                irods::error closedir_err = fileClosedir( _comm, myPath_obj );
                if ( !closedir_err.ok() ) {
                    std::stringstream msg;
                    msg << "fileClosedir for [";
                    msg << myPath;
                    msg << "]";
                    irods::error log_err = PASSMSG( msg.str(), closedir_err );
                    irods::log( log_err );
                }

                return stat_err.code();

            }  // if !stat_err.ok()

            // filter based on stat results: file, dir, or error
            int status = 0;
            if ( ( statbuf.st_mode & S_IFREG ) != 0 ) {
                // handle case where file is found
                irods::error unlink_err = fileUnlink( _comm, myPath_obj );
                status = unlink_err.code();
                if ( !unlink_err.ok() ) {
                    irods::error log_err = PASSMSG( "error during unlink.", unlink_err );
                    irods::log( log_err );
                }
            }
            else if ( ( statbuf.st_mode & S_IFDIR ) != 0 ) {
                // handle case where directory is found
                fileRmdirInp_t myRmdirInp;
                memset( &myRmdirInp, 0, sizeof( myRmdirInp ) );

                myRmdirInp.flags    = _rmdir_inp->flags;
                rstrcpy( myRmdirInp.dirName, myPath, MAX_NAME_LEN );
                rstrcpy( myRmdirInp.rescHier, _rmdir_inp->rescHier, MAX_NAME_LEN );

                // RECURSE - call _rsFileRmdir on this new found directory
                status = _rsFileRmdir( _comm, &myRmdirInp );

            }
            else {
                status = SYS_INVALID_FILE_PATH;
                rodsLog( LOG_NOTICE, "_rsFileRmdir:  for %s, status = %d", myPath, status );
            }

            // handle error condition on the above three cases
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE, "_rsFileRmdir:  rm of %s failed, status = %d", myPath, status );

                // call closedir via resource plugin, handle errors
                irods::error closedir_err = fileClosedir( _comm, myPath_obj );
                if ( !closedir_err.ok() ) {
                    std::stringstream msg;
                    msg << "fileClosedir failed for [";
                    msg << myPath;
                    msg << "]";
                    irods::error log_err = PASSMSG( msg.str(), closedir_err );
                    irods::log( log_err );
                }

                return status;

            } // if status < 0

            // =-=-=-=-=-=-=-
            // continue readdir via resource plugin
            readdir_err = fileReaddir( _comm, coll_obj, &myFileDirent );

        } // while

        // =-=-=-=-=-=-=-
        // call closedir via resource plugin, handle errors
        irods::error closedir_err = fileClosedir( _comm, coll_obj );
        if ( !closedir_err.ok() ) {
            std::stringstream msg;
            msg << "fileClosedir failed for [";
            msg << _rmdir_inp->dirName;
            msg << "]";
            irods::error log_err = PASSMSG( msg.str(), closedir_err );
            irods::log( log_err );
        }

    } // if RMDIR_RECUR ( recursive delete )

    // =-=-=-=-=-=-=-
    // make the call to rmdir via the resource plugin
    irods::error rmdir_err = fileRmdir( _comm, coll_obj );
    if ( !rmdir_err.ok() ) {
        std::stringstream msg;
        msg << "fileRmdir failed for [";
        msg << _rmdir_inp->dirName;
        msg << "]";
        irods::error err = PASSMSG( msg.str(), rmdir_err );
        irods::log( err );
    }

    return rmdir_err.code();

} // _rsFileRmdir
