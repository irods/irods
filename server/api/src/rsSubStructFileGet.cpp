/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileGet.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileGet.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"
#include "irods_error.hpp"
#include "irods_stacktrace.hpp"

int
rsSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile,
                    bytesBuf_t *subFileGetOutBBuf ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileGet( rsComm, subFile, subFileGetOutBBuf );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileGet( rsComm, subFile, subFileGetOutBBuf,
                                         rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileGet: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile,
                        bytesBuf_t *subFileGetOutBBuf, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileGet: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileGet( rodsServerHost->conn, subFile,
                                 subFileGetOutBBuf );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileGet: rcSubStructFileGet failed for %s, status = %d",
                 subFile->subFilePath, status );
    }

    return status;
}

int _rsSubStructFileGet( rsComm_t*   _comm,
                         subFile_t*  _sub_file,
                         bytesBuf_t* _out_buf ) {
    // =-=-=-=-=-=-=-
    // convert subfile to a first class object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    if ( _sub_file->offset <= 0 ) {
        irods::log( ERROR( SYS_INVALID_INPUT_PARAM, "invalid length" ) );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // open the structured object
    irods::error open_err = fileOpen( _comm, struct_obj );
    if ( !open_err.ok() ) {
        std::stringstream msg;
        msg << "fileOpen error for [";
        msg << struct_obj->sub_file_path();
        msg << "], status = ";
        msg << open_err.code();
        irods::log( PASSMSG( msg.str(), open_err ) );
        return open_err.code();
    }

    // =-=-=-=-=-=-=-
    // allocte outgoing buffer if necessary
    if ( _out_buf->buf == NULL ) {
        _out_buf->buf = new unsigned char[ _sub_file->offset ];
    }

    // =-=-=-=-=-=-=-
    // read structured file
    irods::error read_err = fileRead( _comm, struct_obj, _out_buf->buf, _sub_file->offset );
    int status = read_err.code();

    if ( !read_err.ok() ) {
        if ( status >= 0 ) {
            std::stringstream msg;
            msg << "failed in fileRead for [";
            msg << struct_obj->sub_file_path();
            msg << ", toread ";
            msg << _sub_file->offset;
            msg << ", read ";
            msg << read_err.code();
            irods::log( PASSMSG( msg.str(), read_err ) );

            status = SYS_COPY_LEN_ERR;
        }
        else {
            std::stringstream msg;
            msg << "failed in fileRead for [";
            msg << struct_obj->sub_file_path();
            msg << ", status = ";
            msg << read_err.code();
            irods::log( PASSMSG( msg.str(), read_err ) );

            status = read_err.code();
        }

    }
    else {
        _out_buf->len = read_err.code();
    }

    // =-=-=-=-=-=-=-
    // ok, done with that.  close the file.
    irods::error close_err = fileClose( _comm, struct_obj );
    if ( !close_err.ok() ) {
        std::stringstream msg;
        msg << "failed in fileClose for [";
        msg << struct_obj->sub_file_path();
        msg << ", status = ";
        msg << close_err.code();
        irods::log( PASSMSG( msg.str(), read_err ) );
    }

    return status;

}
