/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileFstat.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"

int
rsSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileFstatInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileFstat (rsComm, subStructFileFstatInp, subStructFileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileFstat (rsComm, subStructFileFstatInp, subStructFileStatOut,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileFstat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileFstat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileFstat (rodsServerHost->conn, subStructFileFstatInp,
      subStructFileStatOut);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileFstat: rcFileFstat failed for fd %d",
         subStructFileFstatInp->fd);
    }

    return status;

}

int _rsSubStructFileFstat( rsComm_t*                _comm, 
                           subStructFileFdOprInp_t* _fstat_inp,
                           rodsStat_t**             _stat_out ) {
    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( NULL == _comm      || 
        NULL == _fstat_inp ||
        NULL == _stat_out ) {
        rodsLog( LOG_ERROR, "_rsSubStructFileFstat :: one or more parameters is null" );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // create first class structured object 
    eirods::structured_object struct_obj;
    struct_obj.comm( _comm );
    struct_obj.file_descriptor( _fstat_inp->fd );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to open a file
    struct stat fs;
    eirods::error fstat_err = fileFstat( struct_obj, &fs );
    if( !fstat_err.ok() ) {
        std::stringstream msg;
        msg << "_rsSubStructFileFstat - failed on call to fileFstat for [";
        msg << struct_obj.file_descriptor();
        msg << "]";
        eirods::log( PASS( false, -1, msg.str(), fstat_err ) );
        return fstat_err.code();

    } else {
        // =-=-=-=-=-=-=-
        // convert unix stat struct to an irods stat struct
        if( !*_stat_out ) {
            *_stat_out = new rodsStat_t;
        }
        
        int status = statToRodsStat( *_stat_out, &fs );

        return fstat_err.code();

    }

}

