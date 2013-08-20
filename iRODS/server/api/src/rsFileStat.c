/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileStat.h for a description of this API call.*/

#include "fileStat.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_stacktrace.h"
#include "eirods_resource_backport.h"


int
rsFileStat (rsComm_t *rsComm, fileStatInp_t *fileStatInp, 
            rodsStat_t **fileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileStatOut = NULL;

    //remoteFlag = resolveHost (&fileStatInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileStatInp->rescHier, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "rsFileStat - failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else {
        status = rsFileStatByHost (rsComm, fileStatInp, fileStatOut,
                                   rodsServerHost);
        return (status);
    }
}

int
rsFileStatByHost (rsComm_t *rsComm, fileStatInp_t *fileStatInp,
                  rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost)
{
    int remoteFlag;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "rsFileStatByHost: Input NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    remoteFlag = rodsServerHost->localFlag;

    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileStat (rsComm, fileStatInp, fileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileStat (rsComm, fileStatInp, fileStatOut,
                                 rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileStat: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileStat (rsComm_t *rsComm, fileStatInp_t *fileStatInp,
                rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost)
{   
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileStat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileStat (rodsServerHost->conn, fileStatInp, fileStatOut);

    if (status < 0) { 
        rodsLog (LOG_DEBUG,
                 "remoteFileStat: rcFileStat failed for %s",
                 fileStatInp->fileName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to stat via resource plugin
int _rsFileStat( 
    rsComm_t*      _comm, 
    fileStatInp_t* _stat_inp, 
    rodsStat_t**   _stat_out ) {
    struct stat myFileStat;

    if(_stat_inp->objPath[0] == '\0') {
        std::stringstream msg;
        msg << "Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }

    // =-=-=-=-=-=-=-
    // make call to stat via resource plugin
    eirods::file_object_ptr file_obj( 
                                new eirods::file_object( 
                                    _comm, 
                                    _stat_inp->objPath, 
                                    _stat_inp->fileName, 
                                    _stat_inp->rescHier, 
                                    0, 0, 0 ) );
    eirods::error stat_err = fileStat( _comm, file_obj, &myFileStat );

    // =-=-=-=-=-=-=-
    // log error if necessary
    if( !stat_err.ok() ) {
//        eirods::log(LOG_ERROR, stat_err.result());
        return stat_err.code();
    }

    // =-=-=-=-=-=-=-
    // convert unix stat struct to an irods stat struct
    *_stat_out = (rodsStat_t*)malloc (sizeof (rodsStat_t));
    int status = statToRodsStat (*_stat_out, &myFileStat);
        
    // =-=-=-=-=-=-=-
    // manage error if necessary
    if (status < 0) {
        free (*_stat_out);
        *_stat_out = NULL;
    }

    return status;

} // _rsFileStat




