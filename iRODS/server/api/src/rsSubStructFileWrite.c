/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileWrite.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"
 
// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"


int
rsSubStructFileWrite (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileWriteInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileWrite (rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileWrite (rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileWrite: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileWrite (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileWrite: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileWrite (rodsServerHost->conn, subStructFileWriteInp,
      subStructFileWriteOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileWrite: rcFileWrite failed for fd %d",  
         subStructFileWriteInp->fd);
    }

    return status;
}

int _rsSubStructFileWrite( rsComm_t*                _comm, 
                           subStructFileFdOprInp_t* _write_inp,
                           bytesBuf_t*              _out_buf ) {
    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj;
    struct_obj.addr( _write_inp->addr );
    struct_obj.file_descriptor( _write_inp->fd );
    struct_obj.comm( _comm );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to write
    eirods::error write_err = fileWrite( struct_obj, _out_buf->buf, _out_buf->len );
    if( !write_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileWrite - failed on call to fileWrite for [";
        msg << struct_obj.physical_path();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), write_err ) );
        return write_err.code();

    } else {
        return write_err.code();

    }

}

