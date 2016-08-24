/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileRead.c - server routine that handles the fileRead
 * API
 */

/* script generated code */
#include "fileRead.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileRead.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"

int
rsFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp,
            bytesBuf_t *fileReadOutBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;


    remoteFlag = getServerHostByFileInx( fileReadInp->fileInx,
                                         &rodsServerHost );

    if ( fileReadInp->len > 0 ) {
        if ( fileReadOutBBuf->buf == NULL ) {
            fileReadOutBBuf->buf = malloc( fileReadInp->len );
        }
    }
    else {
        return 0;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        retVal = _rsFileRead( rsComm, fileReadInp, fileReadOutBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        retVal = remoteFileRead( rsComm, fileReadInp, fileReadOutBBuf,
                                 rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileRead: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return retVal;
}

int
remoteFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp,
                bytesBuf_t *fileReadOutBBuf, rodsServerHost_t *rodsServerHost ) {
    int retVal;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileRead: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( retVal = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return retVal;
    }

    fileReadInp->fileInx = convL3descInx( fileReadInp->fileInx );
    retVal = rcFileRead( rodsServerHost->conn, fileReadInp,
                         fileReadOutBBuf );

    if ( retVal < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileRead: rcFileRead failed for %s",
                 FileDesc[fileReadInp->fileInx].fileName );
    }

    return retVal;
}

/* _rsFileRead - this the local version of rsFileRead.
 */

int _rsFileRead(
    rsComm_t*      _comm,
    fileReadInp_t* _read_inp,
    bytesBuf_t*    _read_bbuf ) {
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission
    // when RCAT is available

    // =-=-=-=-=-=-=-
    // call resource plugin for POSIX read
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            FileDesc[_read_inp->fileInx].objPath,
            FileDesc[_read_inp->fileInx].fileName,
            FileDesc[_read_inp->fileInx].rescHier,
            FileDesc[_read_inp->fileInx].fd,
            0, 0 ) );

    irods::error ret = fileRead( _comm,
                                 file_obj,
                                 _read_bbuf->buf,
                                 _read_inp->len );
    // =-=-=-=-=-=-=
    // log an error if the read failed,
    // pass long read error
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "fileRead failed for ";
        msg << file_obj->physical_path();
        msg << "]";
        irods::error err = PASSMSG( msg.str(), ret );
        irods::log( err );
    }
    else {
        _read_bbuf->len = ret.code();
    }

    return ret.code();

} // _rsFileRead
