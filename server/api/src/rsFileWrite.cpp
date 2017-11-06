/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileWrite.c - server routine that handles the fileWrite
 * API
 */

/* script generated code */
#include "fileWrite.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileWrite.hpp"
#include <sstream>

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"

int
rsFileWrite( rsComm_t *rsComm, const fileWriteInp_t *fileWriteInp,
             const bytesBuf_t *fileWriteInpBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx( fileWriteInp->fileInx,
                                         &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        retVal = _rsFileWrite( rsComm, fileWriteInp, fileWriteInpBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        retVal = remoteFileWrite( rsComm, fileWriteInp, fileWriteInpBBuf,
                                  rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileWrite: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    if ( retVal >= 0 ) {
        FileDesc[fileWriteInp->fileInx].writtenFlag = 1;
    }

    return retVal;
}

int
remoteFileWrite( rsComm_t *rsComm, const fileWriteInp_t *fileWriteInp,
                 const bytesBuf_t *fileWriteInpBBuf, rodsServerHost_t *rodsServerHost ) {
    int retVal;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileWrite: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( retVal = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return retVal;
    }

    const fileWriteInp_t remoteFileWriteInp{
        .fileInx = convL3descInx( fileWriteInp->fileInx ),
        .len = fileWriteInp->len
    };
    retVal = rcFileWrite( rodsServerHost->conn, &remoteFileWriteInp,
                          fileWriteInpBBuf );

    if ( retVal < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileWrite: rcFileWrite failed for %s",
                 FileDesc[remoteFileWriteInp.fileInx].fileName );
    }

    return retVal;
}

// =-=-=-=-=-=-=-
// _rsFileWrite - this the local version of rsFileWrite.
int _rsFileWrite(
    rsComm_t*       _comm,
    const fileWriteInp_t* _write_inp,
    const bytesBuf_t*     _write_bbuf ) {
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission
    // when RCAT is available

    // =-=-=-=-=-=-=-
    // make a call to the resource write
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            FileDesc[_write_inp->fileInx].objPath,
            FileDesc[_write_inp->fileInx].fileName,
            FileDesc[_write_inp->fileInx].rescHier,
            FileDesc[_write_inp->fileInx].fd,
            0, 0 ) );

    irods::error write_err = fileWrite( _comm,
                                        file_obj,
                                        _write_bbuf->buf,
                                        _write_inp->len );
    // =-=-=-=-=-=-=-
    // log error if necessary
    if ( !write_err.ok() ) {
        std::stringstream msg;
        msg << "fileWrite for [";
        msg << file_obj->physical_path();
        msg << "]";
        irods::error err = PASSMSG( msg.str(), write_err );
        irods::log( err );
    }

    return write_err.code();

} // _rsFileWrite
