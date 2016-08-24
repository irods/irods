/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileTruncate.h for a description of this API call.*/

#include "fileTruncate.h"
#include "miscServerFunct.hpp"
#include "rsFileTruncate.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int
rsFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileTruncateInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileTruncateInp->resc_hier_, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( " failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileTruncate( rsComm, fileTruncateInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileTruncate( rsComm, fileTruncateInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileTruncate: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp,
                    rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileTruncate: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileTruncate( rodsServerHost->conn, fileTruncateInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileTruncate: rcFileTruncate failed for %s, status = %d",
                 fileTruncateInp->fileName, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which makes the call to truncate via the resource plugin
int _rsFileTruncate(
    rsComm_t*      _comm,
    fileOpenInp_t* _trunc_inp ) {
    // =-=-=-=-=-=-=-
    // make the call to rename via the resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _trunc_inp->objPath,
            _trunc_inp->fileName,
            _trunc_inp->resc_hier_,
            0, 0, 0 ) );
    file_obj->size( _trunc_inp->dataSize );

    // =-=-=-=-=-=-=-
    // pass condInput
    file_obj->cond_input( _trunc_inp->condInput );

    irods::error trunc_err = fileTruncate( _comm, file_obj );

    // =-=-=-=-=-=-=-
    // report errors if any
    if ( !trunc_err.ok() ) {
        std::stringstream msg;
        msg << "fileTruncate for [";
        msg << _trunc_inp->fileName;
        msg << "]";
        msg << trunc_err.code();
        irods::error err = PASSMSG( msg.str(), trunc_err );
        irods::log( err );
    }

    return trunc_err.code();

} // _rsFileTruncate
