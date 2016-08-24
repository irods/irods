/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileLseek.h for a description of this API call.*/

#include "fileLseek.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileLseek.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"

int
rsFileLseek( rsComm_t *rsComm, fileLseekInp_t *fileLseekInp,
             fileLseekOut_t **fileLseekOut ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    *fileLseekOut = NULL;

    remoteFlag = getServerHostByFileInx( fileLseekInp->fileInx,
                                         &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        retVal = _rsFileLseek( rsComm, fileLseekInp, fileLseekOut );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        retVal = remoteFileLseek( rsComm, fileLseekInp,
                                  fileLseekOut, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileLseek: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return retVal;
}

int
remoteFileLseek( rsComm_t *rsComm, fileLseekInp_t *fileLseekInp,
                 fileLseekOut_t **fileLseekOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileLseek: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fileLseekInp->fileInx = convL3descInx( fileLseekInp->fileInx );
    status = rcFileLseek( rodsServerHost->conn, fileLseekInp, fileLseekOut );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileLseek: rcFileLseek failed for %d, status = %d",
                 fileLseekInp->fileInx, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to stat via resource plugin
int _rsFileLseek(
    rsComm_t*        _comm,
    fileLseekInp_t*  _lseek_inp,
    fileLseekOut_t** _lseek_out ) {
    // =-=-=-=-=-=-=-
    // make call to lseek via resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            FileDesc[_lseek_inp->fileInx].objPath,
            FileDesc[_lseek_inp->fileInx].fileName,
            FileDesc[_lseek_inp->fileInx].rescHier,
            FileDesc[_lseek_inp->fileInx].fd,
            0, 0 ) );

    irods::error lseek_err = fileLseek(
                                 _comm,
                                 file_obj,
                                 _lseek_inp->offset,
                                 _lseek_inp->whence );
    // =-=-=-=-=-=-=-
    // handle error conditions and log
    if ( !lseek_err.ok() ) {
        std::stringstream msg;
        msg << "lseek failed for [";
        msg << FileDesc[_lseek_inp->fileInx].fileName;
        msg << "]";
        irods::error ret_err = PASSMSG( msg.str(), lseek_err );
        irods::log( ret_err );

        return lseek_err.code();

    }
    else {
        ( *_lseek_out ) = ( fileLseekOut_t* )malloc( sizeof( fileLseekOut_t ) );
        memset( ( *_lseek_out ), 0, sizeof( fileLseekOut_t ) );

        ( *_lseek_out )->offset = lseek_err.code();

        return 0;
    }

} // _rsFileLseek
