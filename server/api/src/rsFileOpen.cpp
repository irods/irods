/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileOpen.c - server routine that handles the fileOpen
 * API
 */

/* script generated code */
#include "fileOpen.h"
#include "fileOpr.hpp"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileOpen.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int
rsFileOpen( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;

    //remoteFlag = resolveHost (&fileOpenInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileOpenInp->resc_hier_, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }


    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else {
        fileInx = rsFileOpenByHost( rsComm, fileOpenInp, rodsServerHost );
        return fileInx;
    }
}

int
rsFileOpenByHost( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
                  rodsServerHost_t *rodsServerHost ) {
    int fileInx;
    int fd;
    int remoteFlag;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsFileOpenByHost: Input NULL rodsServerHost" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remoteFlag = rodsServerHost->localFlag;

    if ( remoteFlag == LOCAL_HOST ) {
        fd = _rsFileOpen( rsComm, fileOpenInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        fd = remoteFileOpen( rsComm, fileOpenInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileOpenByHost: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    if ( fd < 0 ) {
        return fd;
    }
    fileInx = allocAndFillFileDesc( rodsServerHost, fileOpenInp->objPath, fileOpenInp->fileName, fileOpenInp->resc_hier_,
                                    fd, fileOpenInp->mode );

    return fileInx;
}

int
remoteFileOpen( rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
                rodsServerHost_t *rodsServerHost ) {
    int fileInx;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpen: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fileInx = rcFileOpen( rodsServerHost->conn, fileOpenInp );

    if ( fileInx < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpen: rcFileOpen failed for %s",
                 fileOpenInp->fileName );
    }

    return fileInx;
}

// =-=-=-=-=-=-=-
// _rsFileOpen - this the local version of rsFileOpen.
int _rsFileOpen(
    rsComm_t*      _comm,
    fileOpenInp_t* _open_inp ) {
    // =-=-=-=-=-=-=-
    // pointer check
    if ( !_comm || !_open_inp ) {
        rodsLog( LOG_ERROR, "_rsFileOpen - null comm or open_inp pointer(s)." );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // NOTE:: need to check resource permission and vault permission
    //        when RCAT is available
    if ( ( _open_inp->flags & O_WRONLY ) && ( _open_inp->flags & O_RDWR ) ) {
        /* both O_WRONLY and O_RDWR are on, can cause I/O to fail */
        _open_inp->flags &= ~( O_WRONLY );
    }

    // =-=-=-=-=-=-=-
    // call file open on the resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _open_inp->objPath,
            _open_inp->fileName,
            _open_inp->resc_hier_,
            0,
            _open_inp->mode,
            _open_inp->flags ) );
    file_obj->in_pdmo( _open_inp->in_pdmo );

    // =-=-=-=-=-=-=-
    // pass condInput
    file_obj->cond_input( _open_inp->condInput );

    irods::error ret_err = fileOpen( _comm, file_obj );

    // =-=-=-=-=-=-=-
    // log errors, if any.
    // NOTE:: direct archive access is a special case in that we
    //        need to detect it and then carry on
    if ( ret_err.code() == DIRECT_ARCHIVE_ACCESS ) {
        return DIRECT_ARCHIVE_ACCESS;

    }
    else if ( !ret_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileOpen: fileOpen for [";
        msg << _open_inp->fileName;
        msg << "]";
        irods::error out_err = PASSMSG( msg.str(), ret_err );
        irods::log( out_err );
        return ret_err.code();

    } // if

    return file_obj->file_descriptor();

} // _rsFileOpen
