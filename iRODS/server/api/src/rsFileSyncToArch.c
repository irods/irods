/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileSyncToArch.c - server routine that handles the fileSyncToArch
 * API
 */

/* script generated code */
#include "fileSyncToArch.h"
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
rsFileSyncToArch (rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

//    remoteFlag = resolveHost (&fileSyncToArchInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileSyncToArchInp->rescHier, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else {
        status = rsFileSyncToArchByHost (rsComm, fileSyncToArchInp, rodsServerHost);
        return (status);
    }
}

int 
rsFileSyncToArchByHost (rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, rodsServerHost_t *rodsServerHost)
{
    int status;
    int remoteFlag;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "rsFileSyncToArchByHost: Input NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
            
    remoteFlag = rodsServerHost->localFlag;
    
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileSyncToArch (rsComm, fileSyncToArchInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileSyncToArch (rsComm, fileSyncToArchInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileSyncToArchByHost: resolveHost returned value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteFileSyncToArch (rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileSyncToArch: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcFileSyncToArch (rodsServerHost->conn, fileSyncToArchInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileSyncToArch: rcFileSyncToArch failed for %s",
                 fileSyncToArchInp->filename);
    }

    return status;
}

// =-=-=-=-=-=-=-=
// _rsFileSyncToArch - this the local version of rsFileSyncToArch.
int _rsFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp) {
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission
    // when RCAT is available 
    
    // =-=-=-=-=-=-=-
    // prep 
    if(fileSyncToArchInp->objPath[0] == '\0') {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }
    
    // =-=-=-=-=-=-=-
    // make call to synctoarch via resource plugin
    eirods::file_object file_obj( rsComm,
                                  fileSyncToArchInp->objPath,
                                  fileSyncToArchInp->filename, "", 0, 
                                  fileSyncToArchInp->mode, 
                                  fileSyncToArchInp->flags );
    file_obj.resc_hier( fileSyncToArchInp->rescHier );
    eirods::error sync_err = fileSyncToArch( rsComm, file_obj, fileSyncToArchInp->cacheFilename );

    if( !sync_err.ok() ) {

        if (getErrno (sync_err.code()) == ENOENT) {
            // =-=-=-=-=-=-=-
            // the directory does not exist, lets make one
            mkDirForFilePath( rsComm,"/", fileSyncToArchInp->filename, getDefDirMode() );
        } else if (getErrno (sync_err.code()) == EEXIST) {
            // =-=-=-=-=-=-=-
            // an empty dir may be there, make the call to rmdir via the resource plugin
            eirods::collection_object coll_obj( fileSyncToArchInp->filename, fileSyncToArchInp->rescHier, 0, 0 );
            eirods::error rmdir_err = fileRmdir( rsComm, coll_obj );
            if( !rmdir_err.ok() ) {
                std::stringstream msg;
                msg << "fileRmdir failed for [";
                msg << fileSyncToArchInp->filename;
                msg << "]";
                eirods::error err = PASSMSG( msg.str(), sync_err );
                eirods::log ( err );
            }
        } else {
            std::stringstream msg;
            msg << "fileSyncToArch failed for [";
            msg << fileSyncToArchInp->filename;
            msg << "]";
            eirods::error err = PASSMSG( msg.str(), sync_err );
            eirods::log ( err );
            return sync_err.code();
        }
        
        // =-=-=-=-=-=-=-
        // make call to synctoarch via resource plugin
        sync_err = fileSyncToArch( rsComm, file_obj, fileSyncToArchInp->cacheFilename );
        if( !sync_err.ok() ) {
            std::stringstream msg;
            msg << "fileSyncToArch failed for [";
            msg << fileSyncToArchInp->filename;
            msg << "]";
            msg << sync_err.code();
            eirods::error err = PASSMSG( msg.str(), sync_err );
            eirods::log ( err );
        }

    } // if !sync_err.ok()

    // =-=-=-=-=-=-=-
    // has the file name has changed?
    rstrcpy(fileSyncToArchInp->filename, file_obj.physical_path().c_str(), MAX_NAME_LEN);

    return (sync_err.code());

} // _rsFileSyncToArch
 
