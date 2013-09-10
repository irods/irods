/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileStageToCache.c - server routine that handles the fileStageToCache
 * API
 */

/* script generated code */
#include "fileStageToCache.h"
#include "fileOpr.h"
#include "miscServerFunct.h"
#include "dataObjOpr.h"
#include "physPath.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_stacktrace.h"
#include "eirods_resource_backport.h"

int
rsFileStageToCache (rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileStageToCacheInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileStageToCacheInp->rescHier, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else {
        status = rsFileStageToCacheByHost (rsComm, fileStageToCacheInp, 
                                           rodsServerHost);
        return (status);
    }
}

int 
rsFileStageToCacheByHost (rsComm_t *rsComm, 
                          fileStageSyncInp_t *fileStageToCacheInp, rodsServerHost_t *rodsServerHost)
{
    int status;
    int remoteFlag;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "rsFileStageToCacheByHost: Input NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
            
    remoteFlag = rodsServerHost->localFlag;
    
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileStageToCache (rsComm, fileStageToCacheInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileStageToCache (rsComm, fileStageToCacheInp, 
                                         rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileStageToCacheByHost: resolveHost returned value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteFileStageToCache (rsComm_t *rsComm, 
                        fileStageSyncInp_t *fileStageToCacheInp, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileStageToCache: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcFileStageToCache (rodsServerHost->conn, fileStageToCacheInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileStageToCache: rcFileStageToCache failed for %s",
                 fileStageToCacheInp->filename);
    }

    return status;
}

// =-=-=-=-=-=-=-
// _rsFileStageToCache - this the local version of rsFileStageToCache.
int _rsFileStageToCache(
    rsComm_t*           _comm, 
    fileStageSyncInp_t* _stage_inp ) {

    // XXXX need to check resource permission and vault permission
    // when RCAT is available 

    // =-=-=-=-=-=-=-
    // need to make this now. It will be difficult to do it with
    // parallel I/O
    mkDirForFilePath( _comm, "/", _stage_inp->cacheFilename, getDefDirMode() );

    if(_stage_inp->objPath[0] == '\0') {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }

    // =-=-=-=-=-=-=-
    // make the call to fileStageToCache via the resource plugin
    eirods::file_object_ptr file_obj( 
                                new eirods::file_object( 
                                    _comm,
                                    _stage_inp->objPath,
                                    _stage_inp->filename, "", 0, 
                                    _stage_inp->mode, 
                                    _stage_inp->flags ) );
    file_obj->resc_hier( _stage_inp->rescHier );
    file_obj->size(_stage_inp->dataSize);
    eirods::error stage_err = fileStageToCache( _comm,
                                                file_obj,  
                                                _stage_inp->cacheFilename );
    // =-=-=-=-=-=-=-
    // handle errors if any
    if( !stage_err.ok() ) {
        // =-=-=-=-=-=-=-
        // an empty dir may be there 
        if (getErrno ( stage_err.code() ) == EEXIST) {

            // =-=-=-=-=-=-=-
            // make the call to rmdir via the resource plugin
            eirods::collection_object_ptr coll_obj( 
                                              new eirods::collection_object(
                                                  _stage_inp->cacheFilename, 
                                                  _stage_inp->rescHier, 
                                                  0, 0 ) );
            eirods::error rmdir_err = fileRmdir( _comm, coll_obj );
            if( !rmdir_err.ok() ) {
                std::stringstream msg;
                msg << "fileRmdir failed for [";
                msg << _stage_inp->cacheFilename;
                msg << "]";
                eirods::error err = PASSMSG( msg.str(), rmdir_err );
                eirods::log ( err );
            }
        } else {
            eirods::error err = ASSERT_PASS(stage_err, "Failed for \"%s\".", _stage_inp->filename);
            eirods::log ( err );
        }

        // =-=-=-=-=-=-=-
        // try it again?  ( make the call to fileStageToCache via the resource plugin )
        stage_err = fileStageToCache( _comm,
                                      file_obj, 
                                      _stage_inp->cacheFilename );
        // =-=-=-=-=-=-=-
        // handle errors if any
        if( !stage_err.ok() ) {
            std::stringstream msg;
            msg << "fileStageTocache for [";
            msg << _stage_inp->filename;
            msg << "]";
            msg << stage_err.code();
            eirods::error err = PASSMSG( msg.str(), stage_err );
            eirods::log ( err );
        }

    } // if ! stage_err.ok
    
    return (stage_err.code());

} // _rsFileStageToCache 







