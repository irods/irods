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
#include "eirods_collection_object.h"

int
rsFileStageToCache (rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileStageToCacheInp->addr, &rodsServerHost);

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
int _rsFileStageToCache (rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp ) {

    // XXXX need to check resource permission and vault permission
    // when RCAT is available 
    int status;

    // =-=-=-=-=-=-=-
    // need to make this now. It will be difficult to do it with
    // parallel I/O
    mkDirForFilePath( fileStageToCacheInp->cacheFileType, rsComm, "/", 
	                  fileStageToCacheInp->cacheFilename, getDefDirMode() );

    // =-=-=-=-=-=-=-
    // make the call to fileStageToCache via the resource plugin
    eirods::error stage_err = fileStageToCache( fileStageToCacheInp->filename,  fileStageToCacheInp->cacheFilename, 
                                                fileStageToCacheInp->mode,      fileStageToCacheInp->flags,
                                                fileStageToCacheInp->dataSize, &fileStageToCacheInp->condInput,
											    status );
	// =-=-=-=-=-=-=-
	// handle errors if any
    if( !stage_err.ok() ) {
		// =-=-=-=-=-=-=-
        // an empty dir may be there 
        if (getErrno (status) == EEXIST) {

			// =-=-=-=-=-=-=-
			// make the call to rmdir via the resource plugin
			eirods::collection_object coll_obj( fileStageToCacheInp->cacheFilename, 0, 0 );
			eirods::error rmdir_err = fileRmdir( coll_obj );
			if( !rmdir_err.ok() ) {
				std::stringstream msg;
				msg << "_rsFileStageToCache: fileRmdir for ";
				msg << fileStageToCacheInp->cacheFilename;
				msg << ", status = ";
				msg << rmdir_err.code();
				eirods::error err = PASS( false, rmdir_err.code(), msg.str(), rmdir_err );
				eirods::log ( err );
			}
        } else {
			std::stringstream msg;
			msg << "_rsFileStageToCache: fileStageTocache for ";
			msg << fileStageToCacheInp->filename;
			msg << ", status = ";
			msg << status;
			eirods::error err = PASS( false, status, msg.str(), stage_err );
			eirods::log ( err );
	    }

		// =-=-=-=-=-=-=-
		// try it again?  ( make the call to fileStageToCache via the resource plugin )
        stage_err = fileStageToCache( fileStageToCacheInp->filename, fileStageToCacheInp->cacheFilename,
                                      fileStageToCacheInp->mode,     fileStageToCacheInp->flags,
                                      fileStageToCacheInp->dataSize, &fileStageToCacheInp->condInput,
		                              status );
		// =-=-=-=-=-=-=-
		// handle errors if any
		if( !stage_err.ok() ) {
			std::stringstream msg;
			msg << "_rsFileStageToCache: fileStageTocache for ";
			msg << fileStageToCacheInp->filename;
			msg << ", status = ";
			msg << status;
			eirods::error err = PASS( false, status, msg.str(), stage_err );
			eirods::log ( err );
	    }

    } // if ! stage_err.ok
    
	return (status);

} // _rsFileStageToCache 







