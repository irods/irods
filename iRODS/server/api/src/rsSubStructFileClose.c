/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileClose.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"

int
rsSubStructFileClose (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileCloseInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileClose (rsComm, subStructFileCloseInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileClose (rsComm, subStructFileCloseInp,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileClose: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileClose (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileClose: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileClose (rodsServerHost->conn, subStructFileCloseInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileClose: rcFileClose failed for fd %d",
         subStructFileCloseInp->fd);
    }

    return status;
}

int
_rsSubStructFileClose( rsComm_t*                _comm, 
                       subStructFileFdOprInp_t* _close_inp ) {
    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj;
    struct_obj.comm( _comm );
    struct_obj.file_descriptor( _close_inp->fd );

    struct_obj.resc_hier( eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE );


    // =-=-=-=-=-=-=-
    // call abstrcated interface to open a file
    eirods::error close_err = fileClose( _comm, struct_obj );
    if( !close_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileClose for fd [ ";
        msg << struct_obj.file_descriptor();
        msg << " ]";
        eirods::log( PASSMSG( msg.str(), close_err ) );
        return close_err.code();

    } else {
        return close_err.code();

    }

}


