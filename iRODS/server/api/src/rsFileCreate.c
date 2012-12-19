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

#include <string>

int
rsFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int fd;

    remoteFlag = resolveHost (&fileCreateInp->addr, &rodsServerHost);

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

    fileInx = allocAndFillFileDesc (rodsServerHost, fileCreateInp->fileName, fileCreateInp->resc_hier_, fileCreateInp->fileType, fd,
                                    fileCreateInp->mode);

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
int _rsFileCreate( rsComm_t *rsComm, fileCreateInp_t *fileCreateInp,
                   rodsServerHost_t *rodsServerHost ) {
    std::cerr << "qqq - calling " << __FUNCTION__ << std::endl;
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission when RCAT 
    // is available 
        
    // =-=-=-=-=-=-=-
    // check path permissions before creating the file
    if( fileCreateInp->otherFlags & NO_CHK_PERM_FLAG == 0 ) { // JMC - backport 4758
        int status = chkFilePathPerm( rsComm, fileCreateInp, rodsServerHost, DO_CHK_PATH_PERM ); // JMC - backport 4774
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "_rsFileCreate - chkFilePathPerm returned %d", status );
            return (status);
        }
    }

    std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
    // Save the resource hierarchy so we can determine if it changes
    std::string prev_resc_hier = fileCreateInp->resc_hier_;
    
    // =-=-=-=-=-=-=-
    // dont capture the eirods results in the log here as there may be an issue with
    // needing to create a directory, etc.
    eirods::file_object file_obj( rsComm, fileCreateInp->fileName, fileCreateInp->resc_hier_, 0, fileCreateInp->mode, fileCreateInp->flags );
    std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
    eirods::error create_err = fileCreate( file_obj );

    std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
    // =-=-=-=-=-=-=-
    // if we get a bad file descriptor
    if( !create_err.ok() ) {
    
        // =-=-=-=-=-=-=-
        // check error on fd, did the directory exist?
        if( getErrno ( create_err.code() ) == ENOENT ) {

            // =-=-=-=-=-=-=-
            // the directory didnt exist, make it and then try the create once again.
            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
            mkDirForFilePath( rsComm, "/", file_obj.physical_path().c_str(), getDefDirMode() ); 

            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
            create_err = fileCreate( file_obj );
                                                
            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    

            // =-=-=-=-=-=-=-
            // capture the eirods results in the log as our error mechanism
            // doesnt extend outside this function for now.
            if( !create_err.ok() ) {
                std::stringstream msg;
                msg << "_rsFileCreate: ENOENT fileCreate for ";
                msg << fileCreateInp->fileName;
                msg << ", status = ";
                msg << file_obj.file_descriptor();
                eirods::error ret_err = PASS( false, file_obj.file_descriptor(), msg.str(), create_err );
                eirods::log( ret_err );
            }

        } else if( getErrno( create_err.code() ) == EEXIST ) {
            // =-=-=-=-=-=-=-
            // remove a potentially empty directoy which is already in place
            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
            eirods::collection_object coll_obj( fileCreateInp->fileName, 0, 0 );
            eirods::error rmdir_err = fileRmdir( coll_obj );
            if( !rmdir_err.ok() ) {
                std::stringstream msg;
                msg << "_rsFileCreate: EEXIST 1 fileRmdir for ";
                msg << fileCreateInp->fileName;
                msg << ", status = ";
                msg << rmdir_err.code();
                eirods::error err = PASS( false, rmdir_err.code(), msg.str(), rmdir_err );
                eirods::log ( err );
            }
                         
            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
            create_err = fileCreate( file_obj );
                                                                        
            std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
            // =-=-=-=-=-=-=-
            // capture the eirods results in the log as our error mechanism
            // doesnt extend outside this function for now.
            if( !create_err.ok() ) {
                std::stringstream msg;
                msg << "_rsFileCreate: EEXIST 2 fileCreate for ";
                msg << fileCreateInp->fileName;
                msg << ", status = ";
                msg << file_obj.file_descriptor();
                eirods::error ret_err = PASS( false, file_obj.file_descriptor(), msg.str(), create_err );
                eirods::log( ret_err );
            }

        } else {
            std::stringstream msg;
            msg << "_rsFileCreate: UNHANDLED fileCreate for ";
            msg << fileCreateInp->fileName;
            msg << ", status = ";
            msg << create_err.code();
            eirods::error ret_err = PASS( false, create_err.code(), msg.str(), create_err );
            eirods::log( ret_err );
                
        } // else

    } // if !create_err.ok()

    std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
    if(file_obj.resc_hier() != prev_resc_hier) {
        rstrcpy(fileCreateInp->resc_hier_, file_obj.resc_hier().c_str(), MAX_NAME_LEN);
    }
    std::cerr << "qqq - Here " << __FILE__ << ":" << __LINE__ << std::endl;
    
    return file_obj.file_descriptor();

} // _rsFileCreate 
