/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileClosedir.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"
 
// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"

int
rsSubStructFileClosedir (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileClosedirInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileClosedir (rsComm, subStructFileClosedirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileClosedir (rsComm, subStructFileClosedirInp,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileClosedir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileClosedir (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileClosedir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileClosedir (rodsServerHost->conn, subStructFileClosedirInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileClosedir: rcFileClosedir failed for fd %d",
         subStructFileClosedirInp->fd);
    }

    return status;
}

int _rsSubStructFileClosedir( rsComm_t*                _comm, 
                              subStructFileFdOprInp_t* _close_inp ) {
    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj;
    struct_obj.comm( _comm );
    struct_obj.file_descriptor( _close_inp->fd );
 
    struct_obj.resc_hier( eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to close a file
    eirods::error closedir_err = fileClosedir( _comm, struct_obj );
    if( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileClosedir - failed on call to fileClosedir for [";
        msg << struct_obj.physical_path();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), closedir_err ) );
        return closedir_err.code();

    } else {
        return closedir_err.code();

    }

} 

