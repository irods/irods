/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileClosedir.h for a description of this API call.*/

#include "fileClosedir.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileClosedir.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_collection_object.hpp"

int
rsFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx( fileClosedirInp->fileInx,
                                         &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        retVal = _rsFileClosedir( rsComm, fileClosedirInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        retVal = remoteFileClosedir( rsComm, fileClosedirInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileClosedir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    freeFileDesc( fileClosedirInp->fileInx );

    return retVal;
}

int
remoteFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp,
                    rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileClosedir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileClosedir( rodsServerHost->conn, fileClosedirInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileClosedir: rcFileClosedir failed for %d, status = %d",
                 fileClosedirInp->fileInx, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function for handling call to closedir via resource plugin
int _rsFileClosedir(
    rsComm_t*          _comm,
    fileClosedirInp_t* _closedir_inp ) {
    // =-=-=-=-=-=-=-
    // call closedir via resource plugin, handle errors
    irods::collection_object_ptr coll_obj(
        new irods::collection_object(
            FileDesc[_closedir_inp->fileInx].fileName,
            FileDesc[_closedir_inp->fileInx].rescHier,
            0, 0 ) );
    coll_obj->directory_pointer( reinterpret_cast< DIR* >( FileDesc[_closedir_inp->fileInx].driverDep ) );
    irods::error closedir_err = fileClosedir( _comm, coll_obj );

    if ( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "fileClosedir failed for [";
        msg << FileDesc[_closedir_inp->fileInx].fileName;
        msg << "]";
        irods::error log_err = PASSMSG( msg.str(), closedir_err );
        irods::log( log_err );
    }

    return closedir_err.code();

}  // _rsFileClosedir
