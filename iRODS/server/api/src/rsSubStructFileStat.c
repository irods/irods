/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileStat.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"
 
// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"



int
rsSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileStat (rsComm, subFile, subStructFileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileStat (rsComm, subFile, subStructFileStatOut,
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileStat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileStat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileStat (rodsServerHost->conn, subFile, subStructFileStatOut);

    if (status < 0 && getErrno (status) != ENOENT) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileStat: rcSubStructFileStat failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int _rsSubStructFileStat( rsComm_t*    _comm, 
                          subFile_t*   _sub_file, 
                          rodsStat_t** _stat_out ) {
    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj( *_sub_file );
    struct_obj.comm( _comm );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to stat
    struct stat my_stat;
    eirods::error stat_err = fileStat( struct_obj, &my_stat );

    if( !stat_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileStat - failed on call to fileStat for [";
        msg << struct_obj.physical_path();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), stat_err ) );
        return stat_err.code();

    } else {

        // =-=-=-=-=-=-=-
        // convert unix stat struct to an irods stat struct
        *_stat_out = new rodsStat_t;
        int status = statToRodsStat( *_stat_out, &my_stat );
        
        // =-=-=-=-=-=-=-
        // manage error if necessary
        if( status < 0 ) {
            delete (*_stat_out);
            *_stat_out = NULL;
        }

        return status;

    }

}

