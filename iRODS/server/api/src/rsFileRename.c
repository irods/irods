/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileRename.c - server routine that handles the fileRename
 * API
 */

/* script generated code */
#include "fileRename.h"
#include "miscServerFunct.h"
#include "fileOpr.h"
#include "dataObjOpr.h"
#include "physPath.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h" 


int
rsFileRename (rsComm_t *rsComm, fileRenameInp_t *fileRenameInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileRenameInp->addr, &rodsServerHost);

	if (remoteFlag == LOCAL_HOST) {
	    status = _rsFileRename (rsComm, fileRenameInp, rodsServerHost);
	} else if (remoteFlag == REMOTE_HOST) {
	    status = remoteFileRename (rsComm, fileRenameInp, rodsServerHost);
	} else {
		if (remoteFlag < 0) {
		    return (remoteFlag);
		} else {
			rodsLog( LOG_NOTICE, "rsFileRename: resolveHost returned unrecognized value %d",
			         remoteFlag );
			return (SYS_UNRECOGNIZED_REMOTE_FLAG);
		}
	}

    return (status);
}

int
remoteFileRename (rsComm_t *rsComm, fileRenameInp_t *fileRenameInp, 
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileRename: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcFileRename (rodsServerHost->conn, fileRenameInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileRename: rcFileRename failed for %s",
	  fileRenameInp->newFileName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which makes the call to rename via the resource plugin
int _rsFileRename (rsComm_t *rsComm, fileRenameInp_t *fileRenameInp, rodsServerHost_t *rodsServerHost) {
    // =-=-=-=-=-=-=-
    // FIXME: need to check resource permission and vault permission
    // when RCAT is available 
  
    mkDirForFilePath( fileRenameInp->fileType, rsComm, "/", fileRenameInp->newFileName, getDefDirMode () );

    // =-=-=-=-=-=-=-
	// make the call to rename via the resource plugin
    eirods::file_object file_obj( rsComm, fileRenameInp->oldFileName, 0, 0, 0 );
    eirods::error rename_err = fileRename ( file_obj, fileRenameInp->newFileName );

    // =-=-=-=-=-=-=-
	// report errors if any
    if( !rename_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileRename: fileRename for ";
		msg << fileRenameInp->oldFileName;
		msg << " to ";
		msg << fileRenameInp->newFileName;
		msg << ", status = ";
		msg << rename_err.code();
		eirods::error err = PASS( false, rename_err.code(), msg.str(), rename_err );
		eirods::log ( err );
    }

    return rename_err.code();

} // _rsFileRename
 
