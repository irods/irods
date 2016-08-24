/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileUnlink.h for a description of this API call.*/

#include "fileUnlink.h"
#include "miscServerFunct.hpp"
#include "rsFileUnlink.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int
rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileUnlinkInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileUnlinkInp->rescHier, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileUnlink( rsComm, fileUnlinkInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileUnlink( rsComm, fileUnlinkInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileUnlink: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp,
                  rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileUnlink: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileUnlink( rodsServerHost->conn, fileUnlinkInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileUnlink: rcFileUnlink failed for %s, status = %d",
                 fileUnlinkInp->fileName, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function for calling unlink via resource plugin
int _rsFileUnlink(
    rsComm_t*        _comm,
    fileUnlinkInp_t* _unlink_inp ) {
    if ( _unlink_inp->objPath[0] == '\0' ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - empty logical path.";
        irods::log( LOG_ERROR, msg.str() );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // call unlink via resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _unlink_inp->objPath,
            _unlink_inp->fileName,
            _unlink_inp->rescHier,
            0, 0, 0 ) );
    file_obj->in_pdmo( _unlink_inp->in_pdmo );

    irods::error unlink_err = fileUnlink( _comm, file_obj );

    // =-=-=-=-=-=-=-
    // log potential error message
    if ( unlink_err.code() < 0 ) {
        std::stringstream msg;
        msg << "fileRead failed for [";
        msg << _unlink_inp->fileName;
        msg << "]";
        msg << unlink_err.code();
        irods::error ret_err = PASSMSG( msg.str(), unlink_err );
        irods::log( ret_err );
    }

    return unlink_err.code();

} // _rsFileUnlink
