/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileClose.h for a description of this API call.*/

#include "fileClose.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileClose.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"


int
rsFileClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;
    remoteFlag = getServerHostByFileInx( fileCloseInp->fileInx,
                                         &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        retVal = _rsFileClose( rsComm, fileCloseInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        retVal = remoteFileClose( rsComm, fileCloseInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileClose: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    freeFileDesc( fileCloseInp->fileInx );

    return retVal;
}

int
remoteFileClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp,
                 rodsServerHost_t *rodsServerHost ) {
    int status;
    fileCloseInp_t remFileCloseInp;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileClose: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    memset( &remFileCloseInp, 0, sizeof( remFileCloseInp ) );
    remFileCloseInp.fileInx = convL3descInx( fileCloseInp->fileInx );
    status = rcFileClose( rodsServerHost->conn, &remFileCloseInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileClose: rcFileClose failed for %d, status = %d",
                 remFileCloseInp.fileInx, status );
    }

    return status;
}

int _rsFileClose(
    rsComm_t*       _comm,
    fileCloseInp_t* _close_inp ) {
    //Bounds-check the input descriptor index
    if ( _close_inp->fileInx < 0 || static_cast<std::size_t>(_close_inp->fileInx) >= sizeof( FileDesc ) ) {
        std::stringstream msg;
        msg << "L3 descriptor index (into FileDesc) ";
        msg << _close_inp->fileInx;
        msg << " is out of range.";
        irods::error err = ERROR( BAD_INPUT_DESC_INDEX, msg.str() );
        irods::log(err);
        return err.code();
    }

    // trap bound stream case and close directly
    // this is a weird case from rsExecCmd using
    // the FD table
    if ( 0 == strcmp(
                STREAM_FILE_NAME,
                FileDesc[ _close_inp->fileInx ].fileName ) ) {
        return close( FileDesc[ _close_inp->fileInx ].fd );
    }

    // call the resource plugin close operation
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            FileDesc[ _close_inp->fileInx ].objPath,
            FileDesc[ _close_inp->fileInx ].fileName,
            FileDesc[ _close_inp->fileInx ].rescHier,
            FileDesc[ _close_inp->fileInx ].fd,
            0, 0 ) );
    file_obj->in_pdmo( _close_inp->in_pdmo );

    irods::error close_err = fileClose( _comm, file_obj );
    // log an error, if any
    if ( !close_err.ok() ) {
        std::stringstream msg;
        msg << "fileClose failed for [";
        msg << _close_inp->fileInx;
        msg << "]";
        irods::log(PASSMSG( msg.str(), close_err ));
    }

    return close_err.code();

} // _rsFileClose
