/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileRead.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileRead.hpp"

#include "irods_structured_object.hpp"

int
rsSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
                     bytesBuf_t *subStructFileReadOutBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileReadInp->addr, &rodsServerHost );

    if ( subStructFileReadInp->len > 0 ) {
        if ( subStructFileReadOutBBuf->buf == NULL ) {
            subStructFileReadOutBBuf->buf = malloc( subStructFileReadInp->len );
        }
    }
    else {
        return 0;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileRead( rsComm, subStructFileReadInp, subStructFileReadOutBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileRead( rsComm, subStructFileReadInp, subStructFileReadOutBBuf,
                                          rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileRead: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
                         bytesBuf_t *subStructFileReadOutBBuf, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRead: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileRead( rodsServerHost->conn, subStructFileReadInp,
                                  subStructFileReadOutBBuf );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRead: rcFileRead failed for fd %d",  subStructFileReadInp->fd );
    }

    return status;

}

// =-=-=-=-=-=-=-
// api for file read of a structured file
int _rsSubStructFileRead( rsComm_t*                _comm,
                          subStructFileFdOprInp_t* _read_inp,
                          bytesBuf_t*              _out_buf ) {

    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_read_inp ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _read_inp->resc_hier );
    struct_obj->file_descriptor( _read_inp->fd );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to read a file
    irods::error read_err = fileRead(
                                _comm,
                                struct_obj,
                                _out_buf->buf,
                                _read_inp->len );
    if ( !read_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileRead for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), read_err ) );
        _out_buf->len = 0;
        return read_err.code();

    }
    else {
        _out_buf->len = read_err.code();
        return read_err.code();

    }

} // _rsSubStructFileRead
