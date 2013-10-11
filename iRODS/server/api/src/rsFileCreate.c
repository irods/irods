/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileCreate.c - server routine that handles the fileCreate
 * API
 */

/* script generated code */
#include "fileCreate.h"
#include "fileOpr.h"
#include "miscServerFunct.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "rsGlobalExtern.h"
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_stacktrace.h"
#include "eirods_resource_backport.h"


#include <string>

int
rsFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int fd;

    //remoteFlag = resolveHost (&fileCreateInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileCreateInp->resc_hier_, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if (remoteFlag == LOCAL_HOST) {
        fd = _rsFileCreate (rsComm, fileCreateInp, rodsServerHost);
    } else if (remoteFlag == REMOTE_HOST) {
        fd = remoteFileCreate (rsComm, fileCreateInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileCreate: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    if (fd < 0) {
        return (fd);
    }

    fileInx = allocAndFillFileDesc (rodsServerHost, fileCreateInp->objPath, fileCreateInp->fileName, fileCreateInp->resc_hier_,
                                    fileCreateInp->fileType, fd, fileCreateInp->mode);

    return (fileInx);
}

int
remoteFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp, 
                  rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileCreate: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileInx = rcFileCreate (rodsServerHost->conn, fileCreateInp);

    if (fileInx < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileCreate: rcFileCreate failed for %s",
                 fileCreateInp->fileName);
    }

    return fileInx;
}

// =-=-=-=-=-=-=-
// _rsFileCreate - this the local version of rsFileCreate.
int _rsFileCreate( 
    rsComm_t*         _comm, 
    fileCreateInp_t*  _create_inp,
    rodsServerHost_t* _server_host ) {
    // =-=-=-=-=-=-=-
    // NOTE:: need to check resource permission and vault permission when RCAT 
    // is available 
        
    // =-=-=-=-=-=-=-
    // check path permissions before creating the file
    if( ( _create_inp->otherFlags & NO_CHK_PERM_FLAG ) == 0 ) { // JMC - backport 4758
        int status = chkFilePathPerm( 
                         _comm, 
                         _create_inp, 
                         _server_host, 
                         DO_CHK_PATH_PERM ); // JMC - backport 4774
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "_rsFileCreate - chkFilePathPerm returned %d", status );
            return (status);
        }
    }

    if(_create_inp->objPath[0] == '\0') {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }
    
    // =-=-=-=-=-=-=-
    // dont capture the eirods results in the log here as there may be an issue with
    // needing to create a directory, etc.
    eirods::file_object_ptr file_obj( new eirods::file_object( 
                                _comm, 
                                _create_inp->objPath, 
                                _create_inp->fileName, 
                                _create_inp->resc_hier_, 
                                0, 
                                _create_inp->mode, 
                                _create_inp->flags ) );
    file_obj->in_pdmo(_create_inp->in_pdmo);
    
    eirods::error create_err = fileCreate( _comm, file_obj );

    // =-=-=-=-=-=-=-
    // if we get a bad file descriptor
    if( !create_err.ok() ) {
        // =-=-=-=-=-=-=-
        // check error on fd, did the directory exist?
        if( getErrno ( create_err.code() ) == ENOENT ) {

            // =-=-=-=-=-=-=-
            // the directory didnt exist, make it and then try the create once again.
            int status = mkDirForFilePath( 
                             _comm, 
                             "/", 
                             file_obj->physical_path().c_str(), 
                             getDefDirMode() ); 
            if(status != 0) {
                std::stringstream msg;
                msg << "Unable to make directory: \"" 
                    << file_obj->physical_path() 
                    << "\"";
                eirods::log(LOG_ERROR, msg.str());
                return status;
            }
            
            create_err = fileCreate( _comm, file_obj );
                                                
            // =-=-=-=-=-=-=-
            // capture the eirods results in the log as our error mechanism
            // doesnt extend outside this function for now.
            if( !create_err.ok() ) {
                std::stringstream msg;
                msg << "ENOENT fileCreate for [";
                msg << _create_inp->fileName;
                msg << "]";
                eirods::error ret_err = PASSMSG( msg.str(), create_err );
                eirods::log( ret_err );

            }

        } else if( getErrno( create_err.code() ) == EEXIST ) {
            // =-=-=-=-=-=-=-
            // remove a potentially empty directoy which is already in place
            eirods::collection_object_ptr coll_obj( 
                                              new eirods::collection_object( 
                                                      _create_inp->fileName, 
                                                      _create_inp->resc_hier_, 
                                                      0, 0 ) );
            eirods::error rmdir_err = fileRmdir( _comm, coll_obj );
            if( !rmdir_err.ok() ) {
                std::stringstream msg;
                msg << "EEXIST 1 fileRmdir for [";
                msg << _create_inp->fileName;
                msg << "]";
                eirods::error err = PASSMSG( msg.str(), rmdir_err );
                eirods::log ( err );
            }
                         
            create_err = fileCreate( _comm, file_obj );
                                                                        
            // =-=-=-=-=-=-=-
            // capture the eirods results in the log as our error mechanism
            // doesnt extend outside this function for now.
            if( !create_err.ok() ) {
                std::stringstream msg;
                msg << "EEXIST 2 fileCreate for [";
                msg << _create_inp->fileName;
                msg << "]";
                eirods::error ret_err = PASSMSG( msg.str(), create_err );
                eirods::log( ret_err );
            }

        } else {
            std::stringstream msg;
            msg << "UNHANDLED fileCreate for [";
            msg << _create_inp->fileName;
            msg << "]";
            eirods::error ret_err = PASSMSG( msg.str(), create_err );
            eirods::log( ret_err );
                
        } // else

    } // if !create_err.ok()

    rstrcpy(
        _create_inp->resc_hier_, 
        file_obj->resc_hier().c_str(), 
        MAX_NAME_LEN );
    rstrcpy(
        _create_inp->fileName, 
        file_obj->physical_path().c_str(), 
        MAX_NAME_LEN );

    return file_obj->file_descriptor();

} // _rsFileCreate 
