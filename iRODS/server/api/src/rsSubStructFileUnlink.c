/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileUnlink.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"
 
// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"


int
rsSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileUnlink (rsComm, subFile);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileUnlink (rsComm, subFile, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileUnlink: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileUnlink: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileUnlink (rodsServerHost->conn, subFile);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileUnlink: rcSubStructFileUnlink failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int _rsSubStructFileUnlink( rsComm_t*  _comm, 
                            subFile_t* _sub_file ) {
    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj( *_sub_file );
    struct_obj.comm( _comm );

    struct_obj.resc_hier( eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to unlink
    eirods::error unlink_err = fileUnlink( _comm, struct_obj );
    if( !unlink_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileUnlink - failed on call to fileUnlink for [";
        msg << struct_obj.physical_path();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), unlink_err ) );
        return unlink_err.code();

    } else {
        return unlink_err.code();

    }

}

