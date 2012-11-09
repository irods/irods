/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileOpen.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"



int
rsSubStructFileOpen (rsComm_t *rsComm, subFile_t *subFile)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        fd = _rsSubStructFileOpen (rsComm, subFile);
    } else if (remoteFlag == REMOTE_HOST) {
        fd = remoteSubStructFileOpen (rsComm, subFile, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileOpen: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (fd);
}

int
remoteSubStructFileOpen (rsComm_t *rsComm, subFile_t *subFile,
rodsServerHost_t *rodsServerHost)
{
    int fd;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileOpen: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fd = rcSubStructFileOpen (rodsServerHost->conn, subFile);

    if (fd < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileOpen: rcSubStructFileOpen failed for %s, status = %d",
          subFile->subFilePath, fd);
    }

    return fd;
}

int
_rsSubStructFileOpen( rsComm_t*  _comm, 
                      subFile_t* _sub_file ) {

    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj( *_sub_file );
    struct_obj.comm( _comm );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to open a file
    eirods::error open_err = fileOpen( struct_obj );
    if( !open_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileOpen - failed on call to fileOpen for [";
        msg << struct_obj.sub_file_path();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), open_err ) );
        return open_err.code();

    } else {
        return open_err.code();

    }

} // _rsSubStructFileOpen


