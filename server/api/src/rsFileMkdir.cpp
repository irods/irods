/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileMkdir.h for a description of this API call.*/

#include "fileMkdir.h"
#include "miscServerFunct.hpp"
#include "rsFileMkdir.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_collection_object.hpp"
#include "irods_stacktrace.hpp"
#include "fileDriver.hpp"

int
rsFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &fileMkdirInp->addr, &rodsServerHost );
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileMkdir( rsComm, fileMkdirInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileMkdir( rsComm, fileMkdirInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileMkdir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp,
                 rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileMkdir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileMkdir( rodsServerHost->conn, fileMkdirInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpen: rcFileMkdir failed for %s",
                 fileMkdirInp->dirName );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to mkdir via resource plugin
int _rsFileMkdir(
    rsComm_t*       _comm,
    fileMkdirInp_t* _mkdir_inp ) {
    // =-=-=-=-=-=-=-
    // make call to mkdir via resource plugin

    irods::collection_object_ptr coll_obj(
        new irods::collection_object(
            _mkdir_inp->dirName,
            _mkdir_inp->rescHier,
            _mkdir_inp->mode,
            0 ) );

    // =-=-=-=-=-=-=-
    // pass condInput
    coll_obj->cond_input( _mkdir_inp->condInput );

    irods::error mkdir_err = fileMkdir( _comm, coll_obj );

    // =-=-=-=-=-=-=-
    // log error if necessary
    if ( !mkdir_err.ok() ) {
        if ( getErrno( mkdir_err.code() ) != EEXIST ) {
            std::stringstream msg;
            msg << "fileMkdir failed for ";
            msg << _mkdir_inp->dirName;
            msg << "]";
            irods::error ret_err = PASSMSG( msg.str(), mkdir_err );
            irods::log( ret_err );
        }
    }

    return mkdir_err.code();

} // _rsFileMkdir
