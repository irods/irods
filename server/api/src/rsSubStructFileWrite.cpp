/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileWrite.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileWrite.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"


int
rsSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
                      bytesBuf_t *subStructFileWriteOutBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileWriteInp->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileWrite( rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileWrite( rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf,
                                           rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileWrite: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
                          bytesBuf_t *subStructFileWriteOutBBuf, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileWrite: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileWrite( rodsServerHost->conn, subStructFileWriteInp,
                                   subStructFileWriteOutBBuf );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileWrite: rcFileWrite failed for fd %d",
                 subStructFileWriteInp->fd );
    }

    return status;
}

int _rsSubStructFileWrite(
    rsComm_t*                _comm,
    subStructFileFdOprInp_t* _write_inp,
    bytesBuf_t*              _out_buf ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_write_inp ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _write_inp->resc_hier );
    struct_obj->file_descriptor( _write_inp->fd );
    struct_obj->addr( _write_inp->addr );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to write
    irods::error write_err = fileWrite(
                                 _comm,
                                 struct_obj,
                                 _out_buf->buf,
                                 _out_buf->len );
    if ( !write_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileWrite for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), write_err ) );
        return write_err.code();

    }
    else {
        return write_err.code();

    }

}
