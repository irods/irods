/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileChmod.h for a description of this API call.*/

#include "fileChmod.h"
#include "miscServerFunct.hpp"
#include "rsFileChmod.hpp"


// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int
rsFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileChmodInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileChmodInp->rescHier, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileChmod( rsComm, fileChmodInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileChmod( rsComm, fileChmodInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileChmod: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp,
                 rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileChmod: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileChmod( rodsServerHost->conn, fileChmodInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpen: rcFileChmod failed for %s",
                 fileChmodInp->fileName );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which makes the call to chmod via the resource plugin
int _rsFileChmod(
    rsComm_t*         _comm,
    fileChmodInp_t*   _chmod_inp ) {
    // =-=-=-=-=-=-=-
    // make the call to chmod via the resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _chmod_inp->objPath,
            _chmod_inp->fileName,
            _chmod_inp->rescHier,
            0, 0, 0 ) );
    irods::error chmod_err = fileChmod( _comm, file_obj, _chmod_inp->mode );

    // =-=-=-=-=-=-=-
    // report errors if any
    if ( !chmod_err.ok() ) {
        std::stringstream msg;
        msg << "fileChmod failed for [";
        msg << _chmod_inp->fileName;
        msg << "] to mode [";
        msg << _chmod_inp->mode;
        msg << "";
        irods::error err = PASSMSG( msg.str(), chmod_err );
        irods::log( err );
    }

    return chmod_err.code();
} //_rsChmod
