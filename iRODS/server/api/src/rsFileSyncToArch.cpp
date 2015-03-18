/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileSyncToArch.c - server routine that handles the fileSyncToArch
 * API
 */

/* script generated code */
#include "fileSyncToArch.hpp"
#include "fileOpr.hpp"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int rsFileSyncToArch(
    rsComm_t*           rsComm,
    fileStageSyncInp_t* fileSyncToArchInp,
    fileSyncOut_t**     sync_out ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    ( *sync_out ) = ( fileSyncOut_t* )malloc( sizeof( fileSyncOut_t ) );
    bzero( ( *sync_out ), sizeof( fileSyncOut_t ) );

//    remoteFlag = resolveHost (&fileSyncToArchInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileSyncToArchInp->rescHier, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else {
        status = rsFileSyncToArchByHost( rsComm, fileSyncToArchInp, sync_out, rodsServerHost );
        return status;
    }
}

int
rsFileSyncToArchByHost( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** sync_out, rodsServerHost_t *rodsServerHost ) {
    int status;
    int remoteFlag;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsFileSyncToArchByHost: Input NULL rodsServerHost" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remoteFlag = rodsServerHost->localFlag;

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileSyncToArch( rsComm, fileSyncToArchInp, sync_out );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileSyncToArch( rsComm, fileSyncToArchInp, sync_out, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileSyncToArchByHost: resolveHost returned value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** sync_out, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileSyncToArch: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcFileSyncToArch( rodsServerHost->conn, fileSyncToArchInp, sync_out );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileSyncToArch: rcFileSyncToArch failed for %s",
                 fileSyncToArchInp->filename );
    }

    return status;
}

// =-=-=-=-=-=-=-=
// _rsFileSyncToArch - this the local version of rsFileSyncToArch.
int _rsFileSyncToArch(
    rsComm_t*           _comm,
    fileStageSyncInp_t* _sync_inp,
    fileSyncOut_t**     _sync_out ) {
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission
    // when RCAT is available

    // =-=-=-=-=-=-=-
    // prep
    if ( _sync_inp->objPath[0] == '\0' ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        irods::log( LOG_ERROR, msg.str() );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // make call to synctoarch via resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _sync_inp->objPath,
            _sync_inp->filename, "", 0,
            _sync_inp->mode,
            _sync_inp->flags ) );
    file_obj->resc_hier( _sync_inp->rescHier );

    // =-=-=-=-=-=-=-
    // pass condInput
    file_obj->cond_input( _sync_inp->condInput );

    irods::error sync_err = fileSyncToArch( _comm, file_obj, _sync_inp->cacheFilename );

    if ( !sync_err.ok() ) {

        if ( getErrno( sync_err.code() ) == ENOENT ) {
            // =-=-=-=-=-=-=-
            // the directory does not exist, lets make one
            int status = mkDirForFilePath(
                _comm,
                0,
                _sync_inp->filename,
                _sync_inp->rescHier,
                getDefDirMode() );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR, "mkDirForFilePath failed in _rsFileSyncToArch with status %d", status );
                return status;
            }
        }
        else if ( getErrno( sync_err.code() ) == EEXIST ) {
            // =-=-=-=-=-=-=-
            // an empty dir may be there, make the call to rmdir via the resource plugin
            irods::collection_object_ptr coll_obj(
                new irods::collection_object(
                    _sync_inp->filename,
                    _sync_inp->rescHier,
                    0, 0 ) );
            coll_obj->cond_input( _sync_inp->condInput );
            irods::error rmdir_err = fileRmdir( _comm, coll_obj );
            if ( !rmdir_err.ok() ) {
                std::stringstream msg;
                msg << "fileRmdir failed for [";
                msg << _sync_inp->filename;
                msg << "]";
                irods::error err = PASSMSG( msg.str(), sync_err );
                irods::log( err );
            }
        }
        else {
            std::stringstream msg;
            msg << "fileSyncToArch failed for [";
            msg << _sync_inp->filename;
            msg << "]";
            irods::error err = PASSMSG( msg.str(), sync_err );
            irods::log( err );
            return sync_err.code();
        }

        // =-=-=-=-=-=-=-
        // make call to synctoarch via resource plugin
        sync_err = fileSyncToArch( _comm, file_obj, _sync_inp->cacheFilename );
        if ( !sync_err.ok() ) {
            std::stringstream msg;
            msg << "fileSyncToArch failed for [";
            msg << _sync_inp->filename;
            msg << "]";
            msg << sync_err.code();
            irods::error err = PASSMSG( msg.str(), sync_err );
            irods::log( err );
        }

    } // if !sync_err.ok()

    // =-=-=-=-=-=-=-
    // has the file name has changed?
    if ( *_sync_out ) {
        rstrcpy( ( *_sync_out )->file_name, file_obj->physical_path().c_str(), MAX_NAME_LEN );
    }

    return sync_err.code();

} // _rsFileSyncToArch

