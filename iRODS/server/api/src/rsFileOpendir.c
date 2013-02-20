/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileOpendir.c - server routine that handles the fileOpendir
 * API
 */

/* script generated code */
#include "fileOpendir.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_collection_object.h"
#include "eirods_resource_backport.h"

int
rsFileOpendir (rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int status;
    void *dirPtr = NULL;

    //remoteFlag = resolveHost (&fileOpendirInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileOpendirInp->resc_hier_, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "rsFileOpendir - failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if (remoteFlag == LOCAL_HOST) {
	    status = _rsFileOpendir (rsComm, fileOpendirInp, &dirPtr);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileOpendir (rsComm, fileOpendirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileOpendir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    if (status < 0) {
	    return (status);
    }
    
    fileInx = allocAndFillFileDesc( rodsServerHost, fileOpendirInp->objPath, fileOpendirInp->dirName, fileOpendirInp->resc_hier_,
				    fileOpendirInp->fileType, status, 0);
    FileDesc[fileInx].driverDep = dirPtr;

    return (fileInx);
}

int
remoteFileOpendir (rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp, 
rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileOpendir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileInx = rcFileOpendir (rodsServerHost->conn, fileOpendirInp);

    if (fileInx < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileOpendir: rcFileOpendir failed for %s",
	  fileOpendirInp->dirName);
    }

    return fileInx;
}

// =-=-=-=-=-=-=-
// _rsFileOpendir - this the local version of rsFileOpendir.
int _rsFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp, void **dirPtr ) {
    // =-=-=-=-=-=-=-
    // FIXME:: XXXX need to check resource permission and vault permission
    // when RCAT is available 
     
    // =-=-=-=-=-=-=-
	// make the call to opendir via resource plugin
    eirods::collection_object coll_obj( fileOpendirInp->dirName, 0, 0 );
    eirods::error opendir_err = fileOpendir( rsComm, coll_obj );

    // =-=-=-=-=-=-=-
	// log an error, if any
    if( !opendir_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileOpendir: fileOpendir for ";
		msg <<fileOpendirInp->dirName; 
		msg << ", status = ";
		msg << opendir_err.code();
		eirods::error err = PASS( false, opendir_err.code(), msg.str(), opendir_err );
		eirods::log ( err );
	}

    (*dirPtr) = coll_obj.directory_pointer(); // JMC -- TEMPORARY 

    return (opendir_err.code());

} // _rsFileOpendir
 
