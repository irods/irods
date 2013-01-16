/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFilePut.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"
#include "eirods_log.h"

int
rsSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFilePut (rsComm, subFile, subFilePutOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFilePut (rsComm, subFile, subFilePutOutBBuf,
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFilePut: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFilePut: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFilePut (rodsServerHost->conn, subFile, 
      subFilePutOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFilePut: rcSubStructFilePut failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int
_rsSubStructFilePut( rsComm_t*   _comm, 
                     subFile_t*  _sub_file,
                     bytesBuf_t* _out_buf ) {
    int status = -1;
    int fd     = -1;
   
    // =-=-=-=-=-=-=-
    // create a structured object on which to operate    
    eirods::structured_object struct_obj( *_sub_file );
    struct_obj.comm( _comm );

    // =-=-=-=-=-=-=-
    // force the opening of a file?
    if (_sub_file->flags & FORCE_FLAG) {
        eirods::error err = fileOpen( struct_obj );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "_rsSubStructFilePut - failed on call to fileCreate for [";
            msg << struct_obj.sub_file_path();
            eirods::log( ERROR( -1, msg.str() ) );
            fd = -1;

        } else {
            fd =  err.code();
        }

    } else {
        eirods::error err = fileCreate( struct_obj );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "_rsSubStructFilePut - failed on call to fileCreate for [";
            msg << struct_obj.sub_file_path();
            eirods::log( ERROR( -1, msg.str() ) );
            fd = -1;

        } else {
            fd =  err.code();
        }

    }

    // =-=-=-=-=-=-=-
    // more error trapping, etc.
    if (fd < 0) {
       if (getErrno (fd) == EEXIST) {
            rodsLog (LOG_DEBUG1,
              "_rsSubStructFilePut: filePut for %s, status = %d",
              _sub_file->subFilePath, fd);
        } else {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut: subStructFileOpen error for %s, stat=%d",
              _sub_file->subFilePath, fd);
	}
        return (fd);
    }

    //status = subStructFileWrite (_sub_file->specColl->type, _comm,
    //  fd, _out_buf->buf, _out_buf->len);
    // =-=-=-=-=-=-=-
    // write the buffer to our structured file
    eirods::error write_err = fileWrite( struct_obj, _out_buf->buf, _out_buf->len );
    if( !write_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFilePut - failed on call to fileWrite for [";
        msg << struct_obj.sub_file_path();
        eirods::log( ERROR( -1, msg.str() ) );
        status = write_err.code();

    } else {
        status = write_err.code();

    }

    // =-=-=-=-=-=-=-
    // more error trapping, etc.
    if (status != _out_buf->len) {
       if (status >= 0) {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut:Write error for %s,towrite %d,read %d",
              _sub_file->subFilePath, _out_buf->len, status);
            status = SYS_COPY_LEN_ERR;
        } else {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut: Write error for %s, status = %d",
              _sub_file->subFilePath, status);
        }
    }

    // =-=-=-=-=-=-=-
    // close up our file after writing
    eirods::error close_err = fileClose( struct_obj );
    if( !close_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFilePut - failed on call to fileWrite for [";
        msg << struct_obj.sub_file_path();
        eirods::log( ERROR( -1, msg.str() ) );
        status = close_err.code();

    }

    return (status);

} // _rsSubStructFilePut
 

