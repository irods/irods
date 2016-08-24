/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileGet.h for a description of this API call.*/

#include "fileGet.h"
#include "miscServerFunct.hpp"
#include "rsFileGet.hpp"
#include "rsFileOpen.hpp"

#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_resource_backport.hpp"


/* rsFileGet - Get the content of a small file into a single buffer
 * in fileGetOutBBuf->buf.
 * Return value - int - number of bytes read.
 */

int
rsFileGet( rsComm_t *rsComm, fileOpenInp_t *fileGetInp,
           bytesBuf_t *fileGetOutBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileGetInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileGetInp->resc_hier_, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileGet( rsComm, fileGetInp, fileGetOutBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileGet( rsComm, fileGetInp, fileGetOutBBuf,
                                rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileGet: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteFileGet( rsComm_t *rsComm, fileOpenInp_t *fileGetInp,
               bytesBuf_t *fileGetOutBBuf, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileGet: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }



    status = rcFileGet( rodsServerHost->conn, fileGetInp, fileGetOutBBuf );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileGet: rcFileGet failed for %s",
                 fileGetInp->fileName );
    }

    return status;
}

int _rsFileGet(
    rsComm_t*      _comm,
    fileOpenInp_t* _get_inp,
    bytesBuf_t*    _get_buf ) {

    int fd;
    int len;

    len = _get_inp->dataSize;
    fd  = _rsFileOpen( _comm, _get_inp );

    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "_rsFileGet: fileGet for %s, status = %d",
                 _get_inp->fileName, fd );
        return fd;
    }

    if ( _get_buf->buf == NULL ) {
        _get_buf->buf = malloc( len );
    }

    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _get_inp->objPath,
            _get_inp->fileName,
            _get_inp->resc_hier_,
            fd,
            _get_inp->mode,
            _get_inp->flags ) );

    // =-=-=-=-=-=-=-
    // pass condInput
    file_obj->cond_input( _get_inp->condInput );

    irods::error read_err = fileRead( _comm,
                                      file_obj,
                                      _get_buf->buf,
                                      len );
    int bytes_read = read_err.code();
    if ( bytes_read != len ) {
        if ( bytes_read >= 0 ) {

            _get_buf->len = bytes_read;

        }
        else {
            std::stringstream msg;
            msg << "fileRead failed for [";
            msg << _get_inp->fileName;
            msg << "]";
            irods::error ret_err = PASSMSG( msg.str(), read_err );
            irods::log( ret_err );
        }
    }
    else {
        _get_buf->len = bytes_read;
    }

    // =-=-=-=-=-=-=-
    // call resource plugin close
    irods::error close_err = fileClose( _comm,
                                        file_obj );
    if ( !close_err.ok() ) {
        irods::error err = PASSMSG( "error on close", close_err );
        irods::log( err );
    }

    return bytes_read;

} // _rsFileGet
